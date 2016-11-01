#pragma once
#include "FrequentInfrequentTask.h"
#include "MIGRAS.h"
#include "bit_matrix.h"

namespace PosetDualization{

/*!
    Разделение всех элементов на t-частые и t-редкие
*/
struct FrequentInfrequentDivision
{
    int threshold;                       ///< порог частоты встречаемости
    
    PosetItemsMatrix maximalFrequent;    ///< максимальные t-частые элементы
    
    PosetItemsMatrix minimalInfrequent;  ///< минимальные t-редкие элементы
    
    bool complete;                       ///< полное описание разделения

    FrequentInfrequentDivision(int t = 1)
        :threshold(t), complete(false)
    {
    }

    void Clear(int n = 0)
    {
        maximalFrequent.Clear(n);
        minimalInfrequent.Clear(n);
    }

};


typedef std::vector<FrequentInfrequentDivision> FrequentThresholdDivisions;


struct FrequentInfrequentEnumeratorOptions
{
    int maxIndepCount;
    int minIndepCount;
    int maxDelay;

    FrequentInfrequentEnumeratorOptions()
        :maxIndepCount(100), minIndepCount(1), maxDelay(2000)
    {
    }
};

class FrequentInfrequentEnumerator
    :public ISolver
{

    FrequentInfrequentTask const* _task;

    PosetDualization::MIGRAS::DualizationAlgorithm _dualizer;

    PosetsDualizationTask _dualTask;

    bit_vector _supportMask;

    PosetItems _current;
    
    PosetItems _forCover;

    PosetItems _indep;

    PosetItems _bounds;

    PosetItems _enumerated1;

    PosetItems _enumerated2;

    PosetItems _enumerated3;

	std::vector<PosetItems> _nonsupportItems;

    std::vector<int> _support;

	//! Булевая матрица, с отметками несравнимых координат
	bit_matrix _supportBitMap;

public:

    FrequentThresholdDivisions Results;

    FrequentInfrequentEnumeratorOptions Options;
        
    void Enumerate(FrequentInfrequentTask const& task);

    virtual void Solve( ITask* task )
    {
        throw std::exception("The method or operation is not implemented.");
    }

    virtual std::string GetResultDescription() const
    {
        throw std::exception("The method or operation is not implemented.");
    }

private:

    void FindMaximalFrequent(int threshold, PosetItems& current);

    
	void FindMinimalInfrequent(int threshold, PosetItems& current);

	void SetSupportedToMinimalIndependent(PosetItems &current);

	void SetUnsupportedToMinimal(PosetItems &current);

    /*!
        Увеличить частоту встречаемоти в пределах допустимого порога
    */
    void IncreaseFrequency(int threshold, PosetItems& current);

	

    /*!
        Уменьшить число удовлетворяющих строк, увеличив одну из координат
    */
    void DecreaseFrequency(int threshold, PosetItems& current, std::vector<int>& support);
    
    void DoEnumerate(FrequentInfrequentDivision& division);

    void AddResults(PosetItemsMatrix const& items);

    void AddMaximalFrequent(FrequentInfrequentDivision& division,
        PosetItems& items);

    void AddMinimalInfrequent(FrequentInfrequentDivision& division,
        PosetItems& items);
    
    int CountSupportLevel( int threshold, PosetItems const& current, std::vector<int> const& unsupport );

	int BuildSupportBitMap(PosetItems const& items);

	int UpdateSupportBitMap(int j, PosetItem const& a);

};

}