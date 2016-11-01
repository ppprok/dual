#pragma once

#include <vector>

#include "marked.h"
#include "bit_chunk.h"
#include "DualizationBacktrackAlgorithmBase.h"


namespace AOT1
{

    //typedef raw_vector<marked<range>> marked_ranges;
    typedef std::vector<bit_chunk> rows;
    typedef rows::iterator rows_iterator;
    typedef rows::const_iterator rows_citerator;

    enum AlgorithmType
    {
        AO1,
        AO2,
        AO1K,
        AO2K,
        AO1M,
        AO2M
    };


    // Локальное состояние алгоритма типа AOT1
    class LocalState
    {

        BOOST_MOVABLE_BUT_NOT_COPYABLE(LocalState)

        friend class GlobalState;

        enum AddResult
        {
            Inner,
            Cover,
            Uncompatable,
            Uncoverable
        };

    private:

        // Непокрытые строки
        rows _uncovered;

        // Конкурентные строки
        rows _competing;
                
        // Оставшиеся столбцы
        bit_chunk _restCols;

    public:

        LocalState(){};

        LocalState(LocalState&& s);

        LocalState& operator =(LocalState&& s);

        // Инициализировать локальное состояние
        void Init(GlobalState& g);

        AddResult AddItem(GlobalState& g, LocalState const& l, int i, int j);

        void RemCol(int j);
                
        bool IsCover() const;

        // Получить набор оставшихся столбцов
        bit_chunk GetRestCols() const;

        // Вернуть ненулевые столбцы
        bit_chunk CollectRestCols();

        // Получить первую непокрытую
        bit_chunk GetFrontUncovered() const;
                
        // Найти минимальную непокрытую строку
        bit_chunk GetMinUncovered(int mc = 0) const;

        // Удалить охватывающие строки
        void RemoveDependentRows();

        // Удалить охватывающие строки и нулевые столбцы и получить оставшиеся столбцы
        bit_chunk GetNonZeroCols(GlobalState& g);

        bit_chunk GetCols(GlobalState& g, AlgorithmType alg);
    };


    // Глобальное состояние алгоритма типа AOT1
    class GlobalState
		:public DualizationBacktrackAlgorithmBase
    {
    private:

        friend class LocalState;

        typedef std::vector<LocalState> LocalStates;
        
        bit_matrix _tempMatrix;

        bit_chunk _tempRow;

        LocalStates _states;

        AlgorithmType _type;

    public:

        GlobalState(AlgorithmType type = AlgorithmType::AO1M)
			: _type(type)
        {}
                        
        void Dualize();

    private:

        bool DoDualize(int stateNum);
				
		LocalState* GetLocalState(int stateNum);
    };


}