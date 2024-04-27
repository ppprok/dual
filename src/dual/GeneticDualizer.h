#pragma once
#include "bit_chunk.h"
#include "bit_matrix.h"
#include "IteratorRange.h"
#include "Randomizer.h"
#include "bit_vector.h"
#include "blob.h"

typedef float Cost;

typedef float Gene;

typedef blob<Gene> Genes;

typedef float Weight;

// ����� ������������� ���������
struct Individual {
    // ������� ��������� ����� ���������� ����������
    Genes rowsOrder;

    // ������� ��������� �������� ���������� ����������
    Genes colsOrder;

    // ������������ ���� �������� (����� ��������, � �������� ���������� ����������)
    bit_vector fixed;

    // ��������, ��������� � �������� ������ �������
    bit_vector cover;

    // ��� ���������� ��������, ������������ ������� ��������
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

// ������� �������, ������������ ��������� ��������
struct ITargetFunc {
    // �������� �������� ��� ��������
    virtual Cost GetCoveredRowsWeight(bit_chunk cover) const = 0;

    // �������� ���������
    virtual void Reset(){};

    // ������������� ����� ��������, ������� ����� �������������� � ����� ��������
    virtual void UpdateFixedColumns(bit_chunk columns){};

    // ������� ����� �����, ��� ������� �������� ��������
    virtual void UpdateUncoveredRows(bit_matrix const& uncovered){};

    virtual ~ITargetFunc(){};
};

// ����� ������������� ���������
struct GAOptions {
    // ����� ��������
    int iterationCount;

    // ����� ������� ������
    int eliteCount;

    // ����� ������ � ���������
    int populationSize;

    // ��������� ������� ��� �������
    Gene mutationScale0;

    // �������� ��������� sigma0 �� �������� � ��������
    Gene mutationScaleScale;

    // �������������� ������� ������� �������� ������� ������� �� ������������� ��� ����������� ������
    Gene crossoverScale0;

    // �������� ��������� crossoverScale0 �� �������� � ��������
    Gene crossoverScaleScale;

    // ����������� �� ����� �����, ��� ������� �������� ��������
    int coverRowsLimit;

    // ����������� �� ����� �����, �� ������� �������������� ������
    int targetRowsLimit;

    // ������������ ����� ������������ �����
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

// ���������� ������
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

// ����� ���������� �����
typedef std::vector<WeightedRow> WeightedRows;

typedef IteratorRange<typename WeightedRows::const_iterator> WeightedRowsRange;

// ��������� ��������� ����������� �����
struct IRowsSource {
    // ����� ��������
    virtual int GetWidth() const = 0;

    // ��������� ��������� ����� �����, ������� ���������� �������
    virtual void LoadUncoveredRows(bit_chunk fixedColumns, int maxCount, WeightedRows& rows) = 0;

    // ������� ���������, ���� �� ��������� ������ ������� ��� ���������� ������
    //virtual void CoverUncoveredRowCallback(WeightedRow const& r){};

    // �������� ��������� ������, �� ������� ������������� ��� ��������, ��� ������ �����, ���� ����� ������ ���.
    //virtual WeightedRow GetNextTargetRow() = 0;

    // ������� ���������, ���� �� ��������� ������ ������� ��� ���������� ������
    //virtual void CoverTargetRowCallback(WeightedRow const& r){};

    // �������� �������� �����
    virtual void Reset(){};

    // ����������� ����������
    virtual ~IRowsSource(){};
};

// ������������ �������� ������ ����������� ��������
class GeneticDualizer : public IConfigurable {

    // ������� ���������
    Individuals _population;

    // ������� �������
    ITargetFunc* _targetFunc;

    // �������� �����
    IRowsSource* _rowsSource;

    // ����� ��������
    int _n;

    // ������, ������� ���������� �������
    WeightedRows _uncoveredRows;

    // ������� �����
    WeightedRows _orderedRows;

    // ������, �� ������� �������������� ��������
    //WeightedRows _targetRows;

    // ����� ��� ���� ������ ����
    bit_vector _commonFixed;

    // ������� ������ ���������
    int _populationSize;

public:
    GAOptions CurrentOptions;

    // ���������������� ������� ��������
    GeneticDualizer();

    // ���������� ������� �������
    void SetTargetFunc(ITargetFunc* target);

    // ���������� �������� �����
    void SetRowsSource(IRowsSource* rowsSource);

    // ���������� �������, � ������� ������ ��������
    //void SetRows(int n, bit_chunks rows);

    // ������� ������ ��������� �� ����������
    void GrowPopulation(int count);

    // ������� ��������� ��������� (� ��� ����� � ������� ������)
    Individuals const& GetPopulation() const;

    // �������� ������� ����� ����� ������
    int GetPopulationSize() const;

    // �������� ������ ������
    void Select(int count);

    // �������� ��������� ���������
    void MakePopulation(int t);

    // �������� ��������� ���������
    void Reset();

    // ��������� ���������� ��������� � �������� �� from �� to-1
    void MakePopulations(int from, int to);

    // �������� �����, ���������� �� ����������� �������� ��������
    void SelectElite();

    void SelectElite(int count);

    // ���������� �����
    virtual void SetOptions(Options const& options);

    // ���������� ����� �� ���������
    virtual void SetDefaultOptions();

    // ��������� ������
    //void LoadUncoveredRows();

    // ��������� ������, �� ������� ����������� ��������
    //void LoadTargetRows();

    // ������������� ��������, ������� �������� ������ � ��������� �����
    void FixCovers();

    // ��������� ����� ����������� ��������
    void Search();

private:
    //void FinallyLoadTargetRows();

    // �������� �������� ������ ����� ��������
    void RandomizeGenesAfterFixCovers();

    // ������� ������� ������, ��������� �� ���������� �����
    //void RemoveDependentTargetRows(WeightedRowsRange const& uncoveredRows);

    // ���������, ��������� �� ������� ������ �����-���� �� ���������� �����
    /*bool IsTargetRowDependentOfUncovered(
        WeightedRow const& tr, 
        WeightedRowsRange const& uncoveredRows) const;*/

    Gene GetCrossoverScale(int t) const;

    Gene GetMutationScale(int t) const;

    // ����� �������� ��� �����
    void FindCover(Individual& ind);

    // ����� ������� � ���������� �����, ����������� ������
    int FindMinWeightColumn(bit_chunk row, Individual const& ind);

    // �������� ��������, ��������� � ������
    void UpdateIndividualCover(Individual& ind);

    // �������� ��� �����
    void UpdateIndividualCost(Individual& ind) const;

    // �������� ����� ����� ��������� �����
    void UpdateIndividual(Individual& ind);

    // ������������� �������� ���� �����
    void RandomIndividual(Individual& ind);

    // ��������� �������� ����������� ������
    void Crossover(Gene scale = 1);

    // ��������� ���� ������
    void Crossover(Genes const& w1, Genes const& w2, Gene c1, Gene c2, Genes& w);

    // ��������� ������
    void Crossover(Individual const& i1, Individual const& i2, Gene c1 /*= 1*/, Gene c2 /*= 1*/, Individual& child);

    // �������� ��� ������, �������������� ��� �����������,
    // �������� �������� ������
    Gene GetCrossoverCost(Gene c1, Gene c2, Gene scale = 1);

    // ������� ���������
    int RemoveDuplicateCovers(Individuals::iterator begin, Individuals::iterator end);

    // ������� �������� �� ������� ���������
    void Mutate(int eliteCount, Gene sigma = 1);

    // ���������� ���� �����
    void Mutate(Individual& ind, Gene sigma = 1);

    // ���������� ����� �����
    void Mutate(Genes& weights, Gene sigma = 1);

    // ����������� ����
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

    // ���� �����
    Weights _rowsWeights;

    // ������, ������� ���������� �������
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

    // ������� ������������ ������
    void RemoveDependentRows(bit_chunks const& masterRows);
    
    // ������� ������ ������ � ������ � ������� �����
    void ClearZeroRows();

    // ������� ������������� ������
    void RemoveDuplicateRows();
};
*/