#pragma once
#include "Interfaces.h"
#include "FeatureSelectorBase.h"


// Простой выбор всех признаков на этапе инициализации
class FTFeatureSelector
    :public FeatureSelectorBase
{
public:    

    virtual void Init( Dataset const& ds );

    virtual void Select(ObjectsClassRange const& cls);

    virtual void Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting );

    static void Register();
    
};