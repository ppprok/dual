
#include "ECmpVotingLearner.h"
#include "GeneticDualizer.h"
#include "../common/errors.h"
#include "ECmpByRankFeatureSelector.h"
#include "ECmpVoting.h"
#include "BoolMatrixPreparation.h"


bit_matrix ECmpVotingLearner::BuildCmpMatrix( 
    ObjectsRange positive, 
    ObjectsRange optional, 
    ObjectsRange negative,
    ECmpSet const& localBasis, 
    Weights& weights)
{
    bit_matrix cmp;
    int m1 = positive.size();
    int m2 = optional.size();
    int m3 = negative.size();

    int m = (m1 + m2) * m3;
    int n1 = localBasis.size();
    int n = n1 + m2;
    cmp.zero(m, n);
    weights.resize(m);

    int i = 0;
    for (auto& po : positive)
    {
        for (auto& no : negative)
        {
            int j = 0;
            for (auto& ecmp : _localBasis)
            {
                if (! _localBasis[j].Apply(*po, * no))
                    cmp.set(i, j);
                ++j;
            }
            weights[i++] = no->mark;                        
        }
    }

    int k = 0;
    for (auto& oo : optional)    {

        for (auto& no : negative)
        {
            int j = 0;
            for (auto& ecmp : _localBasis)
            {
                if (! _localBasis[j].Apply(*oo, *no))
                    cmp.set(i, j);
                ++j;
            }

            cmp.set(i, k + n1);

            weights[i++] = no->mark;                                    
        }
        ++k;
    }

    return std::move(cmp);
}

IBinaryClassifierPtr ECmpVotingLearner::Train( ObjectsClass const& cls, InformativityCondition const& informativity )
{
    const_cast<ObjectsClass&>(cls).SplitOptimal(informativity, _correct, _fitting, 1);
    return Train(_correct, _fitting);
}

IBinaryClassifierPtr ECmpVotingLearner::Train( ObjectsClass const& correct, ObjectsClass const& fitting)
{
    CollectFittingObjects(fitting);

    SelectFeatures(correct);
        

    auto ecmpsets = BuildECmpSets(correct.positive, _optionalObjects, correct.negative, _fittingObjects, _localBasis, CurrentOptions.ecmpCount);

    return std::make_shared<ECmpVoting>(std::move(ecmpsets), join(correct.positive, _optionalObjects));
}


void ECmpVotingLearner::Reset()
{
}

/*OptionsDescription const& ECmpVotingLearner::GetOptionsDescription() const
{
    static OptionsDescription desc = CreateOptionsDescription();
    return desc;
}*/

void ECmpVotingLearner::SetOptions( Options const& options )
{
    if (options.count("ga"))
    {
        _geneticAlgorithm.SetOptions(options.get_child("ga"));
    }
    else
    {
        _geneticAlgorithm.SetDefaultOptions();
    }

    
    ECmpVotingLearnerOptions def;
    
    CurrentOptions.ecmpCount = options.get("ecmpCount", def.ecmpCount);
    CurrentOptions.optionalCount = options.get("optionalCount", def.optionalCount);
    CurrentOptions.fittingLimit = options.get("fittingLimit", def.fittingLimit);

    _featureSelector = Registrator<IFeatureSelectorPtr>::Instance().
                        Create(options.get("featureSelector.type", ""));

    auto conf = ConfigurableCast(_featureSelector);
    if (conf)
        conf->SetOptions(options.get_child("featureSelector"));
    
}

void ECmpVotingLearner::SetFeatureSelector( IFeatureSelectorPtr selector )
{
    _featureSelector = selector;
}

IFeatureSelectorPtr ECmpVotingLearner::GetFeatureSelector()
{
    return _featureSelector;
}

/*OptionsDescription ECmpVotingLearner::CreateOptionsDescription() const
{
    OptionsDescription desc("ECmpSetVoting");
    
    ECmpVotingLearnerOptions def;
    
    desc.add_options()
        ("predicates-count", po::value<int>()->default_value(def.ecmpCount), "count of selected predicates")
        ("fitting-limit", po::value<int>()->default_value(def.fittingLimit), "fitting objects number limit")
        ("optional-count", po::value<int>()->default_value(def.optionalCount), "optional objects number")
        ("features-selector", po::value<std::string>()->default_value("AllFeatureSelector"), "feature selector type")
        ("features-selector-config", po::value<std::string>(), "feature selector config file name");

    desc.add(_geneticAlgorithm.GetOptionsDescription());

    return desc;
}*/

void ECmpVotingLearner::SetDefaultOptions()
{
    CurrentOptions = ECmpVotingLearnerOptions();
    _featureSelector = Registrator<IFeatureSelectorPtr>::Instance().TryCreate("AllFeatureSelector");

    auto conf = ConfigurableCast(_featureSelector);
    if (conf)
        conf->SetDefaultOptions();

    _geneticAlgorithm.SetDefaultOptions();
}

void ECmpVotingLearner::Register()
{
    Registrator<IBinaryClassifierLearnerPtr>::Instance().
        RegisterFactory("ECmpVoting", [](){return std::make_shared<ECmpVotingLearner>();});
}

ECmpVotingLearner::ECmpVotingLearner() 
{    
}

ECmpSetInfos ECmpVotingLearner::BuildECmpSets( ObjectsRange positive, ObjectsRange optional, ObjectsRange negative, ObjectsRange fitting, ECmpSet localBasis, int count )
{
    Weights weights;

    bit_matrix cmp = BuildCmpMatrix(positive, optional, negative, localBasis, weights);
    bit_matrix wcmp = BuildCmpMatrix(positive, optional, fitting, localBasis, weights);

    auto rows = cmp.get_rows();
    BoolMatrixPreparation::RemoveDependentRows(rows);

//    _geneticTarget.SetRows(wcmp.get_rows(), std::move(weights), 0);
    //_geneticTarget.RemoveDependentRows(rows);

    GeneticDualizer& ga = _geneticAlgorithm;
    if (ga.CurrentOptions.populationSize < count)
        ga.CurrentOptions.populationSize = count;

    //ga.SetRows(cmp.width(), rows);
    ga.MakePopulations(0, ga.CurrentOptions.iterationCount);
    ga.SelectElite();

    if ((int)ga.GetPopulationSize() < count)
        count = ga.GetPopulationSize();

    std::vector<int> columns;
    int n = localBasis.size();

    ECmpSetInfos generalECmpSet;
    generalECmpSet.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        auto& ind = ga.GetPopulation()[i];
        columns.clear();
        ind.cover.get_bits(columns);

        ECmpSetInfo ecset;
        ecset.ecmpSet.reserve(columns.size());
        ecset.excludedPrecedents.reserve(optional.size());

        for (int j: columns)
        {
            if (j < n)
                ecset.ecmpSet.push_back(localBasis[j]);
            else
                ecset.excludedPrecedents.push_back(*(optional.begin() + (j-n)));
        }

        generalECmpSet.push_back(std::move(ecset));
    }

    LOG_SEV(trace)<< "generalECmpSet.size()=" << generalECmpSet.size();


    return std::move(generalECmpSet);
}

void ECmpVotingLearner::CollectFittingObjects( ObjectsClass const& fitting )
{
    _fittingObjects.clear();
    _fittingObjects.reserve(fitting.positive.size() + fitting.negative.size());
    for (auto& po : fitting.positive)
    {
        po->mark = po->weight;
        _fittingObjects.push_back(po);
    }

    for (auto& no : fitting.negative)
    {
        no->mark = -no->weight;
        _fittingObjects.push_back(no);
    }

    MostObjectsByWeights(_fittingObjects, CurrentOptions.fittingLimit);

    int optional = CurrentOptions.optionalCount;
    if (optional > (int)_fittingObjects.size())
        optional = (int)_fittingObjects.size();

    _optionalObjects.assign(_fittingObjects.begin(), _fittingObjects.begin() + optional);
}

void ECmpVotingLearner::SelectFeatures( ObjectsClass const& correct )
{
    if (! _featureSelector)
        Throw("Feature selector not defined");

    _localBasis = std::move(_featureSelector->SelectECmpSet(correct.positive, correct.negative));

    LOG_SEV(trace) << "_localBasis.size()=" << _localBasis.size();
}