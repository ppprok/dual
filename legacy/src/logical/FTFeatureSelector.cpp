
#include "FTFeatureSelector.h"
#include "Registrator.h"


void FTFeatureSelector::Init( Dataset const& ds )
{
    if (ds.objects.empty())
    {
        _features.clear();
        return;
    }

    int n = ds.objects.front()->values.size();
    _features.reserve(n);
    for (int i = _features.size(); i < n; ++i)
        _features.push_back(i);
    _features.resize(n);
}

void FTFeatureSelector::Register()
{
    Registrator<IFeatureSelectorPtr>::Instance().
        RegisterFactory("FTFeatureSelector", [](){return std::make_shared<FTFeatureSelector>();});

}

void FTFeatureSelector::Select( ObjectsClassRange const& cls )
{
}

void FTFeatureSelector::Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting )
{
}

