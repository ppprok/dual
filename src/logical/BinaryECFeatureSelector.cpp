#include "stdafx.h"

#include "BinaryECFeatureSelector.h"




void BinaryECFeatureSelector::SetupDefaultLearner()
{
    auto ec  = std::make_shared<ECVotingLearner>();
    ec->GetOptions().coverLimit = 1;
    ec->GetOptions().coverThreshold = 10;

    _learner = ec;
}

void BinaryECFeatureSelector::Reset()
{
    if (_learner)
        _learner->Reset();
}

bool Any(EC const& ec, ObjectsRange objects)
{
    for (auto& o : objects)
        if (ec.Apply(*o))
            return true;
    return false;
}

void BinaryECFeatureSelector::CollectECmpSet(
    ObjectsClassRange const& correct)
{
    _ecmpSet.clear();
    _ecmpSet.reserve(_ecset.size());

    for (auto& ec:_ecset)
    {
        if (Any(ec, correct.positive))
            _ecmpSet.emplace_back(ec, ECmp::LE);
        
        if (Any(ec, correct.negative))
            _ecmpSet.emplace_back(std::move(ec), ECmp::GE);
    }

    _ecset.clear();
}

void BinaryECFeatureSelector::Select( ObjectsClassRange const& correct )
{
    ObjectsClassRange fitting;
    Select(correct, fitting);
}

void BinaryECFeatureSelector::Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting )
{
    if (! _learner)
        return;

    _ecset.clear();

    auto cls = _learner->Train(correct, fitting);

    auto ec = std::dynamic_pointer_cast<IECVoting>(cls);
    if (ec)
        ec->GetECSet(_ecset);

    CollectECmpSet(correct);

    LOG_SEV(trace) << "localBasis.size()=" << _ecmpSet.size();
}

void BinaryECFeatureSelector::SetOptions( ::Options const& options )
{
    _learner = TryCreateChild<IBinaryClassifierLearnerPtr>(options, "learner");

    if (! _learner)
        SetupDefaultLearner();
}



void BinaryECFeatureSelector::SetDefaultOptions()
{
    if (! _learner)
        SetupDefaultLearner();
    else
        TrySetDefaultOptions(_learner);}

BinaryECFeatureSelector::BinaryECFeatureSelector()
{
    SetupDefaultLearner();
}

void BinaryECFeatureSelector::Init( Dataset const& ds )
{
    auto needfs = std::dynamic_pointer_cast<INeedFeatureSelector>(_learner);
    if (needfs)
    {
        auto fs = needfs->GetFeatureSelector();
        if (fs)
            fs->Init(ds);
    }
}