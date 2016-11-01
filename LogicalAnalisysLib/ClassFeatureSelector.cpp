#include "stdafx.h"

#include "ClassFeatureSelector.h"




IFeatureSelectorPtr ClassFeatureSelector::CreateFeatureSelector()
{
    auto type = _featureSelectorOptions.get("type", "");
    if (type.empty())
    {
        auto fs = std::make_shared<BinaryECFeatureSelector>();
        fs->SetOptions(_featureSelectorOptions);
        return fs;
    }
    else
    {
        auto fs = Registrator<IFeatureSelectorPtr>::Instance().Create(type);
        TrySetOptions(fs, _featureSelectorOptions);
        return fs;
    }
}

void ClassFeatureSelector::SetDefaultOptions()
{
    _featureSelectors.clear();
    _featureSelectorOptions = ::Options();
}

void ClassFeatureSelector::SetOptions( ::Options const& options )
{
    _featureSelectors.clear();
    _featureSelectorOptions = options.get_child("inner", ::Options());
}

void ClassFeatureSelector::Init( Dataset const& ds )
{
    _featureSelectors.clear();
    for (auto& cls : ds.classes)
    {
        GetFeatureSelector(cls.label)->Init(ds);
    }
}

ECmpSet const& ClassFeatureSelector::GetECmpSet() const
{
    if (_lastfeatureSelector)
        return _lastfeatureSelector->GetECmpSet();
    return _ecmpSet;
}

Features const& ClassFeatureSelector::GetFeatures() const
{
    if (_lastfeatureSelector)
        return _lastfeatureSelector->GetFeatures();
    return _features;
}

IFeatureSelectorPtr ClassFeatureSelector::GetFeatureSelector( int label )
{
    auto pos = _featureSelectors.find(label);
    if (pos == _featureSelectors.end() || ! pos->second)
        return _featureSelectors[label] = CreateFeatureSelector();
    return pos->second;
}

void ClassFeatureSelector::Select( ObjectsClassRange const& cls )
{
    auto fs = GetFeatureSelector(cls.label);
    fs->Select(cls);
    _lastfeatureSelector = fs;
}

void ClassFeatureSelector::Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting )
{
    auto fs = GetFeatureSelector(correct.label);
    fs->Select(correct, fitting);
    _lastfeatureSelector = fs;
}

void ClassFeatureSelector::Reset()
{
    _lastfeatureSelector.reset();
    for (auto& fs:_featureSelectors)
    {
        if (fs.second)
            fs.second->Reset();
    }
}

void ClassFeatureSelector::Register()
{
    Registrator<IFeatureSelectorPtr>::Instance().
        RegisterFactory("ClassFeatureSelector", [](){return std::make_shared<ClassFeatureSelector>();});
}

ClassFeatureSelector::ClassFeatureSelector()
{    
}