#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "Pruning.h"
#include "WeightedCover.h"
#include "Weighters.h"
#include "CoverCollector.h"



// Сбор перечисляемых покрытий
class BitCoverCollectorCallback:
    public IDualizationCallback
{
protected:

    bit_matrix _covers;

    Weights _weights;

public:

    virtual void Call(DualizationNode& node);

    bit_matrix const& GetCovers() const;

    Weights const& GetWeights() const;
    
    bit_matrix MoveCovers();

    Weights MoveWeights();

    void Reset();
};

struct MostCoverCollectorOptions
{
    // Размер, начиная с которого необходимо делать усечение
    int TrimSize;

    // Число оставляемых покрытий
    int MostCount;

    // Случайный порядок
    bool RandomShuffle;

    // Порядок усечения (меньше нуля --- остаются наибольшие по весу, 
    // больше нуля --- остаются наименьшие по весу, 0 --- остаются случайные)
    int TrimOrder;

    // Остановиться после указнного числа покрытий
    //int StopAfter;
    
    MostCoverCollectorOptions(
        int mostCount = 100, 
        int trimSize = 0, 
        int trimOrder = -1, 
        bool randomShuffle = true)
    {
        assert(mostCount > 0);
        if (trimSize < mostCount)
            trimSize = mostCount * 2;

        MostCount = mostCount;
        TrimSize = trimSize;
        TrimOrder = trimOrder;
        RandomShuffle = randomShuffle;
        //StopAfter = -1;
    }
};

// Коллектор, собирающий N лучших покрытий
class MostCoverCollector
    :public CoverCollector
{

    int _counter;

public:

    // Опции усечения
    MostCoverCollectorOptions Options;
    
    // Взвешиватель вершин ДР
    WeighterBase* Weighter;
    
    // Инициализировать объектом, осуществляющим взвешивание узлов ДР (опционально)
    MostCoverCollector(WeighterBase* weighter = 0);

    // Обработать узел ДР
    virtual void Call( DualizationNode& node );
    
    // Выполнить усечение коллекции покрытий до размера из опций
    void Trim();

    // Выполнить усечение коллекции покрытий
    void Trim(int trimSize);
};