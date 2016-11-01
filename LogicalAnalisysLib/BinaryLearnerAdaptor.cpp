#include "stdafx.h"

#include "BinaryLearnerAdaptor.h"
#include "BoostingLearner.h"
#include "ECVotingLearner.h"




void BinaryLearnerAdaptor::SetupDefaultLearner()
{
    auto boosting = std::make_shared<BoostingLearner>();
    auto ecv = std::make_shared<ECVotingLearner>();
    ecv->GetOptions().coverLimit = 1;
    ecv->GetOptions().coverThreshold = 10;
    boosting->positiveLearner = ecv;

    _learner = boosting;
}

void BinaryLearnerAdaptor::SetDefaultOptions()
{
    Options = BinaryLearnerAdaptorOptions();
    
    if (_learner)
        TrySetDefaultOptions(_learner);
    else
        SetupDefaultLearner();    
}

void BinaryLearnerAdaptor::SetOptions( ::Options const& options )
{
    BinaryLearnerAdaptorOptions def;
    Options.fittingLimit = options.get("fittingLimit", def.fittingLimit);
    
    _learner = TryCreateChild<IClassifierLearnerPtr>(options, "learner");
    
    if (! _learner)
        SetupDefaultLearner();
}

void BinaryLearnerAdaptor::Reset()
{
    if (_learner)
        _learner->Reset();
}

IBinaryClassifierPtr BinaryLearnerAdaptor::Train( 
    ObjectsClassRange const& correct, 
    ObjectsClassRange const& fit )
{
    assert(_learner);        

    ObjectsClassRange fitting(fit);

    if ((int)correct.positive.size() < Options.fittingLimit)
        fitting.positive = ObjectsRange(fit.positive, 0, Options.fittingLimit - correct.positive.size());
    
    if ((int)correct.negative.size() < Options.fittingLimit)
        fitting.negative = ObjectsRange(fit.negative, 0, Options.fittingLimit - correct.negative.size());

    _dataset.Clear();
    _dataset.objects.reserve(
        correct.positive.size() + correct.negative.size() + 
        fitting.positive.size() + fitting.negative.size());

    _dataset.objects.insert(_dataset.objects.end(), correct.positive.begin(), correct.positive.end());
    _dataset.objects.insert(_dataset.objects.end(), correct.negative.begin(), correct.negative.end());
    _dataset.objects.insert(_dataset.objects.end(), fitting.positive.begin(), fitting.positive.end());
    _dataset.objects.insert(_dataset.objects.end(), fitting.negative.begin(), fitting.negative.end());

    _dataset.StoreLabels();

    for (auto& o : correct.positive)
        o->label = 0;
    for (auto& o : fitting.positive)
        o->label = 0;
    for (auto& o : correct.negative)
        o->label = 1;
    for (auto& o : fitting.negative)
        o->label = 1;

    _dataset.CollectClasses();

    auto c = _learner->Train(_dataset);

    _dataset.RestoreLabels();

    return std::make_shared<BinaryClassifierAdaptor>(c);
}

BinaryLearnerAdaptor::BinaryLearnerAdaptor()
{
    SetupDefaultLearner();
}

void BinaryLearnerAdaptor::Register()
{
    Registrator<IBinaryClassifierLearnerPtr>::Instance().RegisterFactory
        ("BinaryAdaptor", [](){return std::make_shared<BinaryLearnerAdaptor>();});
}