#pragma once

#include "DualizationBacktrackAlgorithmBase.h"
#include "WeightBoundsPruning.h"


// Усечение ветвей ДР по границам на суммарный вес покрытых строк
class CoveredRowsWeighter
    :public IDualizationCallback
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
    
    // Покрытые строки
    std::vector<CoveredRow> _coveredRows;

    // Зависимые строки
    IntVector _uncoveredRows;

    // Выполнение подсчёта весов покрытых строк
    WeightBoundsPruningCallback _pruner;

public:

    struct Options
    {
        // Строки, которые будут покрываться
        bit_chunks TargetRows;
        
        // Веса строк
        Weights RowsWeights;
        
        // Вес по умолчанию
        Weight DefaultWeight;

        Options()
            :DefaultWeight(0)
        {
        }
        
        UTILITY_MOVE_DEFAULT_MEMBERS(Options, (TargetRows)(DefaultWeight)(RowsWeights));
    };

public:

    CoveredRowsWeighter();

    // Сбросить параметры по умолчанию
    void Reset();

    // Установить вес по умолчанию
    void SetOptions(Options options);
        
    // Пересчитать веса и обрубить ветвь в случае необходимости
    virtual void Call( DualizationNode& node );

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
    
private:
        
    // Вычислить вес нового узла ДР
    void WeightNewNode( DualizationNode &node );

    // Пересчитать вес при возвращении на уровень вверх
    void WeightBacktrackNode( DualizationNode &node );
};


