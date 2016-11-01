#pragma once

#include "DualizationBacktrackAlgorithmBase.h"


// Усечение ветвей ДР по границам на суммарный вес столбцов
class WeightBoundsPruningCallback
    :public IDualizationCallback
{
protected:

    // Веса столбцов
    Weights _weights;

    // Минимальный допустимый вес
    Weight _minWeight;

    // Максимальный допустимый вес
    Weight _maxWeight;

    // Число положительных весов
    Weight _restPositive;

    // Число отрицательных весов
    Weight _restNegative;

    // Динамически изменять нижнюю границу на (максимальный вес - _epsilon)
    bool _dynamicMinBound;

    // Динамически изменять верхнюю границу на (минимальный вес + _epsilon)
    bool _dynamicMaxBound;

    // Отступ динамической границы от минимального (максимального) найденного веса
    Weight _epsilon;

    // Вес столбцов, для которых он не задан в массиве _weights;
    Weight _defaultWeight;

    
public:

    // Обработать покрытие
    void PrunCoverNode( DualizationNode &node );

    WeightBoundsPruningCallback();

    // Сбросить параметры по умолчанию
    void Reset();

    // Установить строгую верхную границу
    void FixMaxBound(Weight bound);

    // Установить строгую нижнюю границу
    void FixMinBound(Weight bound);

    // Искать наименьшее значение
    void TargetToMin();

    // Искать наибольшее значение
    void TargetToMax();

    // Установить допустимый выход за пределы интервала 
    void SetEpsilon(Weight eps);

    // Установить вес по умолчанию
    void SetDefaultWeight(Weight w);

    // Установить веса
    void SetWeights(Weights weights);

    // Получить вес столбца j
    Weight GetWeight(int j) const;

    bool IsDefaultWeight(int j) const;

    // Пересчитать веса и обрубить ветвь в случае необходимости
    virtual void Call( DualizationNode& node );

    void DecRestWeight( Weight w );
    void IncRestWeight( Weight w );
    void PrunInnerNode( DualizationNode &node );
};
