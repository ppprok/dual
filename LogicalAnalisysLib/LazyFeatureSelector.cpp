#include "stdafx.h"

#include "LazyFeatureSelector.h"
#include "BinaryECFeatureSelector.h"




void LazyFeatureSelector::SetupDefaultInner()
{
    _featureSelector = std::make_shared<BinaryECFeatureSelector>();
}

ECmpSet const& LazyFeatureSelector::GetECmpSet() const
{
    if (_featureSelector)
        return _featureSelector->GetECmpSet();
    return _ecmpSet;
}

Features const& LazyFeatureSelector::GetFeatures() const
{
    if (_featureSelector)
        return _featureSelector->GetFeatures();
    return _features;
}

void LazyFeatureSelector::Init( Dataset const& ds )
{
    if (_featureSelector)
        _featureSelector->Init(ds);
}

void LazyFeatureSelector::SetDefaultOptions()
{
    Options = LazyFeatureSelectorOptions();
    if (_featureSelector)
        TrySetDefaultOptions(_featureSelector);
    else
        SetupDefaultInner();
}

void LazyFeatureSelector::SetOptions( ::Options const& options )
{
    LazyFeatureSelectorOptions def;

    Options.defectLimit = options.get("defectLimit", def.defectLimit);
    Options.fittingLimit = options.get("fittingLimit", def.fittingLimit);
    
    _featureSelector = TryCreateChild<IFeatureSelectorPtr>(options, "inner");

    if (! _featureSelector)
        SetupDefaultInner();    
}

bool LazyFeatureSelector::NeedSelect( ObjectsClassRange const &correct )
{
    assert(_featureSelector);

    if (_featureSelector->GetFeatures().empty() &&
        _featureSelector->GetECmpSet().empty())
        return true;

    //int maxc = (int)ceil(correct.positive.size() * correct.negative.size() * Options.defectLimit);

    if (Options.defectLimit == 0 && AnyDefectPairs(*_featureSelector, correct.positive, correct.negative))
        return true;            

    auto defect = WeightDefectPairs(*_featureSelector, correct.positive, correct.negative);
    return defect >= Options.defectLimit;
}

void LazyFeatureSelector::Select( ObjectsClassRange const& correct )
{
    if (_featureSelector && NeedSelect(correct))
        _featureSelector->Select(correct);
}

void ShiftRanges(
        ObjectsRange const& correct, 
        ObjectsRange const& fitting, 
        int correctSize,
        ObjectsRange& newCorrect,
        ObjectsRange& newFitting, 
        ObjectPtrList& buffer)
{
    if (correct.size() >= correctSize)
    {
        newCorrect = correct;
        newFitting = fitting;
        return;
    }
    
    int m = correctSize - correct.size();
    newCorrect = join_ranges(correct, ObjectsRange(fitting, 0, m), buffer);
    newFitting = ObjectsRange(fitting, m, fitting.size());
}


void LazyFeatureSelector::Select( 
    ObjectsClassRange const& correct, 
    ObjectsClassRange const& fitting )
{
    if (! _featureSelector || ! NeedSelect(correct))
        return;
    ObjectsClassRange correct1, fitting1;
    ShiftRanges(correct.positive, fitting.positive, Options.fittingLimit, correct1.positive, fitting1.positive, _fitting.positive);
    ShiftRanges(correct.negative, fitting.negative, Options.fittingLimit, correct1.negative, fitting1.negative, _fitting.negative);
    _featureSelector->Select(correct1, fitting1);
}

void LazyFeatureSelector::Reset()
{
    if (_featureSelector)
        _featureSelector->Reset();
}

LazyFeatureSelector::LazyFeatureSelector()
{
    SetupDefaultInner();
}