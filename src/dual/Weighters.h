#pragma once

#include <vector>

#include "bit_matrix.h"
#include "DualizationBacktrackAlgorithmBase.h"

//////////////////////////////////////////////////////////////////////////
// Базовый класс для взвешивания узлов ДР
//////////////////////////////////////////////////////////////////////////
class WeighterBase
    :virtual public IDualizationCallback
{
protected:    
    
    // Сумма оставшихся положительных весов
    Weight _restPositive;

    // Сумма оставшихся отрицательных весов
    Weight _restNegative;

    // Минимальный допустимый вес
    Weight _minWeight;

    // Максимальный допустимый вес
    Weight _maxWeight;

    // Веса
    Weights _weights;

    // Максимальное число элементов в сумме
    int _weightSumCount;
    
    // Сделать недействительными оценки сумм оставшихся положительных и отрицательных слагаемых
    void InvalidateRestWeights();

    // Проверить, действительны ли оценки сумм оставшихся положительных и отрицательных слагаемых
    bool IsRestWeightsValid() const;   

    // Вычислить оценки сумм оставшихся положительных и отрицательных слагаемых
    void CalculateRestWeights();

    // Увеличить запас суммарного веса
    void IncRestWeight(Weight w);

    // Уменьшить запас суммарного веса
    void DecRestWeight(Weight w);

public:

    WeighterBase();

    void WeightInnerNode( DualizationNode &node );

    void SetWeights(Weights weights);

    Weight GetWeight(int i);

    Weight GetDefaultWeight();

    void Reset();

    void SetMinWeight(Weight w);

    void SetMaxWeight(Weight w);

    Weight GetRestPositive() const;

    Weight GetRestNegative() const;

    // Отсечь ветку ДР по допустимым значениям текущего веса
    bool Prun(Weight current, Weight minWeight, Weight maxWeight) const;

    // Отсечь ветку ДР по допустимым значениям веса узла
    bool Prun(DualizationNode& node, Weight minWeight, Weight maxWeight) const;

    // Отсечь ветку ДР по текущим допустимым значениям веса узла
    bool Prun(DualizationNode& node);
    
    virtual void WeightNewNode(DualizationNode & node) = 0;
    
    virtual void WeightBacktrackNode(DualizationNode & node) = 0;

    void Call(DualizationNode & node);
};

// Взвешивание узлов ДР по суммарному весу столбцов
class ColumnsWeighterCallback
    :public WeighterBase
{

    // Вычислить вес нового узла ДР
    void WeightNewNode( DualizationNode &node );

    // Пересчитать вес при возвращении на уровень вверх
    void WeightBacktrackNode( DualizationNode &node );
};


// Взвешивание узлов ДР по суммарному весу покрытых и непокрытых строк
class CoveredRowWeighterCallback
    :public WeighterBase
{
    // Строки, которые надо покрыть
    bit_chunks _targetRows;

    typedef std::vector<int> IntVector;

    struct CoveredRow
    {
        int i; // Номер строки
        int j; // Номер столбца
        Weight w; // Вес строки
    };

    // Независимые строки
    std::vector<CoveredRow> _coveredRows;

    // Зависимые строки
    IntVector _uncoveredRows;        

public:

    CoveredRowWeighterCallback();

    // Сбросить параметры по умолчанию
    void Reset();
        
    // Установить строки для покрытия
    void SetTargetRows(bit_chunks targetRows);

    // Вычислить вес нового узла ДР
    void WeightNewNode( DualizationNode &node );

    // Пересчитать вес при возвращении на уровень вверх
    void WeightBacktrackNode( DualizationNode &node );

};