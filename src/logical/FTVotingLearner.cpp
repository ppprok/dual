#include "FTVotingLearner.h"
#include "containers.h"
#include "ECmpByRankFeatureSelector.h"
#include "Registrator.h"
#include "FTVoting.h"
#include "BoolMatrixPreparation.h"
#include "FTFeatureSelector.h"
#include "ECmpVoting.h"



IBinaryClassifierPtr FTVotingLearner::Train( 
    ObjectsClassRange const& correct, 
    ObjectsClassRange const& fitting)
{
    _featureSelector->Select(correct, fitting);
    InitFeatureComparer();

    if (! _featureComparer->features.empty())
    {
        return DoTrainFT(correct, fitting);
    }
    else
    {
        InitECmpSetComparer();
        return DoTrainECmp(correct, fitting);
    }

}

void FTVotingLearner::SetDefaultOptions()
{
    _cmpFitMatrixBuilder.SetDefaultOptions();
    _featureSelector = std::make_shared<FTFeatureSelector>();
}


void FTVotingLearner::SetOptions( Options const& options )
{
    _cmpFitMatrixBuilder.SetOptions(options);

    auto fs = options.get("featureSelector.type", "");
    
    if (! fs.empty())
    {
        _featureSelector = Registrator<IFeatureSelectorPtr>::Instance().
            Create(fs);
        auto conf = std::dynamic_pointer_cast<IConfigurable>(_featureSelector);
        if (conf)
            conf->SetOptions(options.get_child("featureSelector"));
    }
    else
    {
        _featureSelector = std::make_shared<FTFeatureSelector>();
    }    
}


void FTVotingLearner::Reset()
{
}


void FTVotingLearner::Register()
{
    Registrator<IBinaryClassifierLearnerPtr>::Instance().RegisterFactory
        ("FTVoting", [](){return std::make_shared<FTVotingLearner>();});
}

FTVotingLearner::FTVotingLearner()
{
    _featureSelector = std::make_shared<FTFeatureSelector>();
    _featureComparer = std::make_shared<FeatureComparer>();
    _ecmpSetComparer = std::make_shared<ECmpSetComparer>();
}


void FTVotingLearner::SetFeatureSelector( IFeatureSelectorPtr selector )
{
    _featureSelector = selector;
}

IFeatureSelectorPtr FTVotingLearner::GetFeatureSelector()
{
    return _featureSelector;
}


void FTVotingLearner::InitFeatureComparer()
{
    
    _featureComparer->features = _featureSelector->GetFeatures();
    _features.clear();
    _features.reserve(_featureComparer->features.size());
}

void FTVotingLearner::InitECmpSetComparer()
{
    _ecmpSetComparer->ecmpSet = _featureSelector->GetECmpSet();
    _ecmpSet.clear();
    _ecmpSet.reserve(_ecmpSetComparer->ecmpSet.size());
}

IBinaryClassifierPtr FTVotingLearner::DoTrainFT( ObjectsClassRange const &correct, ObjectsClassRange const& fitting )
{
    _cmpFitMatrixBuilder.SetComparer(_featureComparer);
    _cmpFitMatrixBuilder.Build(correct, fitting);
    _cmpFitMatrixBuilder.FindCovers();

    auto& covers = _cmpFitMatrixBuilder.GetCoverMatrix().GetCovers().get_rows();

    FeatureTests tests;
    tests.reserve(covers.size());

    for (auto& c: covers)
    {
        _features.clear();
        _featureComparer->Get(c, _features);
        tests.emplace_back(_features);
    }

    LOG_SEV(trace) << 
        "tests.size()=" << tests.size() << 
        "; correct.positive.size()=" << correct.positive.size() << 
        "; correct.negative.size()=" << correct.negative.size();

    return std::make_shared<FTVoting>(std::move(tests), ObjectPtrList( correct.positive.begin(), correct.positive.end()));
}

IBinaryClassifierPtr FTVotingLearner::DoTrainECmp( ObjectsClassRange const &correct, ObjectsClassRange const& fitting )
{
    _cmpFitMatrixBuilder.SetComparer(_ecmpSetComparer);

    _cmpFitMatrixBuilder.Build(correct, fitting);
    _cmpFitMatrixBuilder.FindCovers();

    auto& covers = _cmpFitMatrixBuilder.GetCoverMatrix().GetCovers().get_rows();

    ECmpSets ecmpSets;
    ecmpSets.reserve(covers.size());

    for (auto& c: covers)
    {
        _ecmpSet.clear();
        _ecmpSetComparer->Get(c, _ecmpSet);
        ecmpSets.emplace_back(_ecmpSet);
    }

    LOG_SEV(trace) << 
        "ecmpSets.size()=" << ecmpSets.size() << 
        "; correct.positive.size()=" << correct.positive.size() << 
        "; correct.negative.size()=" << correct.negative.size();

    return std::make_shared<ECmpVoting0>(std::move(ecmpSets), ObjectPtrList(correct.positive.begin(), correct.positive.end()));
}