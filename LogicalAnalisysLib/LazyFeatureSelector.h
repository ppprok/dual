#pragma once
#include "Interfaces.h"
#include "FeatureSelectorBase.h"
#include "Registrator.h"

// Опции отложенного отбора признаков
struct LazyFeatureSelectorOptions
{
    // максимальный процент дефекта
    float defectLimit;

    // Ограничение числа настроечных объектов, используемых при обучении
    int fittingLimit;

    LazyFeatureSelectorOptions()
        :defectLimit(0.01f), fittingLimit(20)
    {
    }
};


// Отложенный отбор признаков.
// Признаки отбираются только если дефектных пар объектов слишком много
class LazyFeatureSelector
    :public FeatureSelectorBase, public IConfigurable
{
    // Реальный отбор признаков
    IFeatureSelectorPtr _featureSelector;

    ObjectsClass _fitting;
    
public:

    LazyFeatureSelectorOptions Options;

    LazyFeatureSelector();

    virtual void Reset();

    virtual void Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting );

    virtual void Select( ObjectsClassRange const& correct );
    
    virtual void SetOptions( ::Options const& options );
    
    virtual void SetDefaultOptions();

    virtual void Init( Dataset const& ds );

    virtual Features const& GetFeatures() const;

    virtual ECmpSet const& GetECmpSet() const;

    static void Register()
    {
        Registrator<IFeatureSelectorPtr>::Instance().
            RegisterFactory("LazyFeatureSelector", [](){return std::make_shared<LazyFeatureSelector>();});

    }


private:

    bool NeedSelect( ObjectsClassRange const &correct );

    void SetupDefaultInner();

};