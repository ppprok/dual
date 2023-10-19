#pragma once
#include "bit_chunk.h"
#include "bit_matrix.h"
//#include "PUNC.h"
#include "Configurable.h"
#include "IteratorRange.h"
#include "Randomizer.h"
#include "bit_vector.h"
#include "blob.h"

typedef float Cost;

typedef float Gene;

typedef blob<Gene> Genes;

typedef float Weight;

// Особь генетического алгоритма
struct Individual {
    // Порядок обработки строк алгоритмом дуализации
    Genes rowsOrder;

    // Порядок обработки столбцов алгоритмом дуализации
    Genes colsOrder;

    // Неизменяемые биты покрытия (набор столбцов, с которого начинается построение)
    bit_vector fixed;

    // Покрытие, найденное в заданном генами порядке
    bit_vector cover;

    // Вес найденного покрытия, возвращаемый целевой функцией
    Cost cost;

    Individual() : cost(0) {}

    Individual(int gm, int gn, int n) : cost(0), rowsOrder(gm), cover(n), colsOrder(gn), fixed(n) {}

    Gene GetRowWeight(int i, int m) const {
        return GetWeight(i, m, rowsOrder);
    }

    Gene GetColWeight(int j, int n) const {
        return GetWeight(j, n, colsOrder);
    }

    Gene GetWeight(int i, int m, Genes const& genes) const;
};

typedef std::vector<Individual> Individuals;

// Целевая функция, определяющая стоимость покрытия
struct ITargetFunc {
    // Получить значение для покрытия
    virtual Cost GetCoveredRowsWeight(bit_chunk cover) const = 0;

    // Сбросить состояние
    virtual void Reset(){};

    // Зафиксировать набор столбцов, которые будут присутствовать в любом покрытии
    virtual void UpdateFixedColumns(bit_chunk columns){};

    // Указать набор строк, для которых строится покрытие
    virtual void UpdateUncoveredRows(bit_matrix const& uncovered){};

    virtual ~ITargetFunc(){};
};

// Опции генетического алгоритма
struct GAOptions {
    // Число итераций
    int iterationCount;

    // Число элитных особей
    int eliteCount;

    // Число особей в популяции
    int populationSize;

    // Начальный разброс при мутации
    Gene mutationScale0;

    // Характер изменения sigma0 от итерации к итерации
    Gene mutationScaleScale;

    // Первоначальный масштаб влияния значения целевой функции на доминирование при скрещивании особей
    Gene crossoverScale0;

    // Характер изменения crossoverScale0 от итерации к итерации
    Gene crossoverScaleScale;

    // Ограничение на число строк, для которых строится покрытие
    int coverRowsLimit;

    // Ограничение на число строк, по которым осуществляется оценка
    int targetRowsLimit;

    // Максимальное число используемых генов
    int genesLimit;

    GAOptions() :
            eliteCount(5),
            populationSize(10),
            iterationCount(10),
            mutationScale0(1.0f),
            mutationScaleScale(0.01f),
            crossoverScale0(1.0f),
            crossoverScaleScale(0.01f),
            coverRowsLimit(20),
            genesLimit(20) {}
};

// Взвешенная строка
struct WeightedRow {
    //int id;
    Weight weight;
    bit_chunk bits;

    WeightedRow(bit_chunk b, Weight w = 0 /*, int id_ = -1*/) {
        bits = b;
        weight = w;
        //id = id_;
    }

    WeightedRow(Weight w = 0 /*, int id_ = -1*/) {
        weight = w;
        //id = id_;
    }

    bool operator<(WeightedRow const& wr) const {
        return weight < wr.weight;
    }
};

// Набор взвешенных строк
typedef std::vector<WeightedRow> WeightedRows;

typedef IteratorRange<typename WeightedRows::const_iterator> WeightedRowsRange;

// Интерфейс источника покрываемых строк
struct IRowsSource {
    // Число столбцов
    virtual int GetWidth() const = 0;

    // Загрузить очередной набор строк, которые необходимо покрыть
    virtual void LoadUncoveredRows(bit_chunk fixedColumns, int maxCount, WeightedRows& rows) = 0;

    // Вернуть результат, была ли последняя строка покрыта уже имеющимися битами
    //virtual void CoverUncoveredRowCallback(WeightedRow const& r){};

    // Получить следующую строку, по которой настраивается вес покрятия, или пустой набор, если строк больше нет.
    //virtual WeightedRow GetNextTargetRow() = 0;

    // Вернуть результат, была ли последняя строка покрыта уже имеющимися битами
    //virtual void CoverTargetRowCallback(WeightedRow const& r){};

    // Сбросить источник строк
    virtual void Reset(){};

    // Виртуальный деструктор
    virtual ~IRowsSource(){};
};

// Генетический алгоритм поиска оптимальных покрытий
class GeneticDualizer : public IConfigurable {

    // Текущая популяция
    Individuals _population;

    // Целевая функция
    ITargetFunc* _targetFunc;

    // Источник строк
    IRowsSource* _rowsSource;

    // Число столбцов
    int _n;

    // Строки, которые необходимо покрыть
    WeightedRows _uncoveredRows;

    // Порядок строк
    WeightedRows _orderedRows;

    // Строки, по которым осуществляется настрока
    //WeightedRows _targetRows;

    // Общие для всех особей биты
    bit_vector _commonFixed;

    // Текущий размер популяции
    int _populationSize;

public:
    GAOptions CurrentOptions;

    // Иниицализировать целевой функцией
    GeneticDualizer();

    // Установить целевую функцию
    void SetTargetFunc(ITargetFunc* target);

    // Установить источник строк
    void SetRowsSource(IRowsSource* rowsSource);

    // Установить матрицу, у которой ищутся покрытия
    //void SetRows(int n, bit_chunks rows);

    // Довести размер популяции до указанного
    void GrowPopulation(int count);

    // Вернуть последнюю популяцию (в том числе и мертвых особей)
    Individuals const& GetPopulation() const;

    // Получить текущее число живых особей
    int GetPopulationSize() const;

    // Отобрать лучших особей
    void Select(int count);

    // Получить следующую популяцию
    void MakePopulation(int t);

    // Сбросить состояние алгоритма
    void Reset();

    // Выполнить построение популяций с номерами от from до to-1
    void MakePopulations(int from, int to);

    // Отобрать элиту, упорядочив по возрастанию целевого значения
    void SelectElite();

    void SelectElite(int count);

    // Установить опции
    virtual void SetOptions(Options const& options);

    // Установить опции по умолчанию
    virtual void SetDefaultOptions();

    // Загрузить строки
    //void LoadUncoveredRows();

    // Загрузить строки, по которым оценивается покрытие
    //void LoadTargetRows();

    // Зафиксировать покрытия, удалить покрытые строки и зугрузить новые
    void FixCovers();

    // Выполнить поиск оптимальных покрытий
    void Search();

private:
    //void FinallyLoadTargetRows();

    // Породить дочерних особей после фиксации
    void RandomizeGenesAfterFixCovers();

    // Удалить целевые строки, зависящие от непокрытых строк
    //void RemoveDependentTargetRows(WeightedRowsRange const& uncoveredRows);

    // Проверить, покрывает ли целевая строка какую-либо из непокрытых строк
    /*bool IsTargetRowDependentOfUncovered(
        WeightedRow const& tr, 
        WeightedRowsRange const& uncoveredRows) const;*/

    Gene GetCrossoverScale(int t) const;

    Gene GetMutationScale(int t) const;

    // Найти покрытие для особи
    void FindCover(Individual& ind);

    // Найти столбец с наименьшим весом, покрывающий строку
    int FindMinWeightColumn(bit_chunk row, Individual const& ind);

    // Обновить покрытие, связанное с особью
    void UpdateIndividualCover(Individual& ind);

    // Обновить вес особи
    void UpdateIndividualCost(Individual& ind) const;

    // Обновить особь после изменения генов
    void UpdateIndividual(Individual& ind);

    // Сгенерировать случайне гены особи
    void RandomIndividual(Individual& ind);

    // Выполнить попарное скрещивание особей
    void Crossover(Gene scale = 1);

    // Скрестить гены особей
    void Crossover(Genes const& w1, Genes const& w2, Gene c1, Gene c2, Genes& w);

    // Скрестить особей
    void Crossover(Individual const& i1, Individual const& i2, Gene c1 /*= 1*/, Gene c2 /*= 1*/, Individual& child);

    // Получить вес особей, использующийся при скрещивании,
    // учитывая качество особей
    Gene GetCrossoverCost(Gene c1, Gene c2, Gene scale = 1);

    // Удалить дубликаты
    int RemoveDuplicateCovers(Individuals::iterator begin, Individuals::iterator end);

    // Создать мутантов из текущей популяции
    void Mutate(int eliteCount, Gene sigma = 1);

    // Мутировать гены особи
    void Mutate(Individual& ind, Gene sigma = 1);

    // Мутировать набор генов
    void Mutate(Genes& weights, Gene sigma = 1);

    // Нормировать веса
    void Normalize(Genes& weights, Gene min, Gene max);

    Individual& GetIndividual(int i);

    //void RemoveFixedCoveredTargetRows();
};

/*
typedef float Weight;

typedef std::vector<Weight> Weights;

class CoveredRowsWeightTargetFunc
    :public ITargetFunc
{

    // Веса строк
    Weights _rowsWeights;

    // Строки, которые необходимо покрыть
    bit_chunks _rows;

    Weight _w0;

public:
    
    virtual Cost GetCost( bit_chunk cover ) const;

    CoveredRowsWeightTargetFunc()
        :_w0(0)
    {
    }    

    void SetRows(
        bit_chunks rows, 
        Weights weights, 
        Weight defaultWeight = 0)
    {
        _rows = std::move(rows);
        _rowsWeights = std::move(weights);
        _rowsWeights.resize(_rows.size(), defaultWeight);
        _w0 = 0;
    }

    // Удалить охватывающие строки
    void RemoveDependentRows(bit_chunks const& masterRows);
    
    // Удалить пустуе строки и строки с нулевым весом
    void ClearZeroRows();

    // Удалить повторяющиеся строки
    void RemoveDuplicateRows();
};
*/