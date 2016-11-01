#pragma once
#include "Interfaces.h"
#include "Configurable.h"



// Базовый класс для отбора признаков
class FeatureSelectorBase
    :public IFeatureSelector
{
protected:

    Features _features;

    ECmpSet _ecmpSet;



public:
        
    virtual void Reset();
        
    
    
    virtual void Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting )
    {};

    virtual void Select( ObjectsClassRange const& cls )
    {};
    
    virtual Features const& GetFeatures() const;

    virtual ECmpSet const& GetECmpSet() const;

    virtual void Init( Dataset const& ds );



//////////////////////////////////////////////////////////////////////////
// Вспомогательные методы отбора признаков
//////////////////////////////////////////////////////////////////////////
public:

    // Проверить, отделяет ли набор эл.кл. один набор прецедентов от другого набора прецедентов
    static bool AnyDefectPairs(IFeatureSelector const& fs, ObjectsRange const& positive, ObjectsRange const& negative);

    // Посчитать число дефектов
    static int CountDefectPairs(IFeatureSelector const& fs, ObjectsRange const& positive, ObjectsRange const& negative, int maxc = -1);

    // Посчитать относительный вес дефектных пар
    static Weight WeightDefectPairs(IFeatureSelector const& fs, ObjectsRange const& positive, ObjectsRange const& negative);

    // Проверить, различаются ли два объекта построенным набором эл.кл. с отношениями
    static bool IsDefectPair(IFeatureSelector const& fs, Object const& po, Object const& no);

    

};