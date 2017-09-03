#include "stdafx.h"

#include <memory>

#include "MultiByBinaryLearner.h"
//#include "Exceptions.h"
#include "ECmpByRankFeatureSelector.h"
#include "BinaryClassifierVoter.h"
#include "Registrator.h"


IClassifierPtr MultiByBinaryLearner::Train( Dataset const& ds0 )
{
    assert(_positiveLearner || _negativeLearner);

    Dataset ds = ds0;

    InitFeatureSelector(ds);

    auto voter = std::make_shared<BinaryClassifierVoter>();
        
    if (ds.classes.empty())
        ds.CollectClasses();

    InformativityCondition informativity;
    informativity.type = InformativityCondition::EmpericalRisk;

    if (CurrentOptions.iterationCount <= 0)
        CurrentOptions.iterationCount = 1;
    
    for (int t = 0; t < CurrentOptions.iterationCount; ++t)
    {
        ds.RandomShuffle();
        auto dd = ds.Split(CurrentOptions.correctPart, 1-CurrentOptions.correctPart);
        auto correct = std::move(dd.first);
        auto fitting = std::move(dd.second);    
        correct.CollectClasses();
        fitting.CollectClasses();
            
        for (auto& c : correct.classes)
        {
            auto& correctClass = const_cast<ObjectsClass&>(c);
            auto& fittingClass = 
                CurrentOptions.correctPart == 1 ? 
                correctClass :
                const_cast<ObjectsClass&>(fitting.GetClass(correctClass.label));
                            
            IBinaryClassifierPtr bincls;

            if (_positiveLearner)
            {
                _positiveLearner->Reset();
                bincls = _positiveLearner->Train(correctClass, fittingClass);
                assert(CheckCorrectness(bincls, correctClass.positive, correctClass.negative));
                if (bincls)
                    voter->AddClassifier(correctClass.label, bincls, 1);
            }

            if (_negativeLearner)
            {
                _negativeLearner->Reset();
                                
                fittingClass.MakeComplement();
                correctClass.MakeComplement();
                
                bincls = _negativeLearner->Train(correctClass, fittingClass);
                assert(CheckCorrectness(bincls, correctClass.positive, correctClass.negative));

                fittingClass.MakeComplement();
                correctClass.MakeComplement();

                if (bincls)
                    voter->AddClassifier(correctClass.label, bincls, -1);
            }
        }

    }

    return voter;
}

void MultiByBinaryLearner::Reset()
{    
    if (_negativeLearner)
        _negativeLearner->Reset();

    if (_positiveLearner)
        _positiveLearner->Reset();
}

void MultiByBinaryLearner::SetDefaultOptions()
{
    CurrentOptions = MultiByBinaryLearnerOptions();
    _positiveLearner = Registrator<IBinaryClassifierLearnerPtr>::Instance().TryCreate("FeatureTestsVoting");
    _negativeLearner = 0;

    auto conf = std::dynamic_pointer_cast<IConfigurable>(_positiveLearner);
    if (conf)
        conf->SetDefaultOptions();
}

/*OptionsDescription const& MultiByBinaryLearner::GetOptionsDescription() const
{
    static OptionsDescription desc = CreateOptionsDescription();
    return desc;
}*/

void MultiByBinaryLearner::SetOptions( Options const& options )
{
    MultiByBinaryLearnerOptions def;
    
    CurrentOptions.iterationCount = options.get("iterationsCount", def.iterationCount);
    CurrentOptions.correctPart = options.get("correctPart", def.correctPart);
    
    auto pos = options.get("positiveLearner.type", "");
    auto neg = options.get("negativeLearner.type", "");

    if (pos.empty())
        _positiveLearner = 0;
    else
        _positiveLearner = Registrator<IBinaryClassifierLearnerPtr>::Instance().Create(pos);

    if (neg.empty())
        _negativeLearner = 0;
    else
        _negativeLearner = Registrator<IBinaryClassifierLearnerPtr>::Instance().Create(neg);

    auto conf = std::dynamic_pointer_cast<IConfigurable>(_positiveLearner);
    if (conf)
        conf->SetOptions(options.get_child("positiveLearner"));

    conf = std::dynamic_pointer_cast<IConfigurable>(_negativeLearner);
    if (conf)
        conf->SetOptions(options.get_child("negativeLearner"));


}

MultiByBinaryLearner::MultiByBinaryLearner()
{
    SetDefaultOptions();
}

void MultiByBinaryLearner::InitFeatureSelector( Dataset const& ds )
{
    auto nfs = dynamic_cast<INeedFeatureSelector*>(_positiveLearner.get());
    IFeatureSelectorPtr fs1;
    if (nfs)
        fs1 = nfs->GetFeatureSelector();

    nfs = dynamic_cast<INeedFeatureSelector*>(_negativeLearner.get());
    IFeatureSelectorPtr fs2;
    if (nfs)
        fs2 = nfs->GetFeatureSelector();

    if (fs2 == fs1)
        fs2 = 0;

    if (fs1)
    {
        fs1->Reset();
        fs1->Init(ds);
    }

    if (fs2)
    {
        fs2->Reset();
        fs2->Init(ds);
    }
}
/*
OptionsDescription MultiByBinaryLearner::CreateOptionsDescription() const
{
    OptionsDescription desc;
    desc.add_options()
        ("iteration-count", po::value<int>()->default_value(1), "iterations count for all classes")
        ("correct-part", po::value<float>()->default_value(0.75), "part of base dataset against fitting dataset")
        ("positive-learner", po::value<std::string>()->default_value("FeatureTestsVoting"), "type of positive binary learner")
        ("positive-learner-config", po::value<std::string>(), "positive binary learner config file name")
        ("negative-learner", po::value<std::string>()->default_value(""), "type of negative binary learner")
        ("negative-learner-config", po::value<std::string>(), "negative binary learner config file name");
    return desc;
}*/

void MultiByBinaryLearner::SetNegativeBinaryLearner( IBinaryClassifierLearnerPtr neg )
{
    _negativeLearner = neg;
}

void MultiByBinaryLearner::SetPositiveBinaryLearner( IBinaryClassifierLearnerPtr pos )
{
    _positiveLearner = pos;
}

void MultiByBinaryLearner::Register()
{
    Registrator<IClassifierLearnerPtr>::Instance().RegisterFactory
        ("MultiByBinary", [](){return std::make_shared<MultiByBinaryLearner>();});
}