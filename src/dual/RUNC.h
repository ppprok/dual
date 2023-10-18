#pragma once

#include <vector>

#include "AOT1.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_chunk.h"
#include "bit_mask.h"
#include "bit_vector.h"
#include "marked.h"
#include "move_default.h"
#include "range.h"

namespace Runc {

typedef std::vector<bit_chunk> rows;
typedef std::vector<marked<range>> marked_ranges;
typedef rows::iterator rows_iterator;
typedef rows::const_iterator rows_citerator;

enum AlgorithmType {
    OPT,
    RUNC,
    RUNCM
};

class GlobalState;

// Локальное состояние алгоритма типа Runc
class LocalState {
    enum AddResult {
        Inner,
        Cover,
        Uncompatable,
        Uncoverable
    };

    /*private:

    BOOST_MOVABLE_BUT_NOT_COPYABLE(LocalState)*/

    friend class GlobalState;

private:
    // Непокрытые строки
    rows _uncovered;

    // Опорные строки
    rows _support;

    // Группы опорных векторов
    marked_ranges _supportGroups;

    // Оставшиеся столбцы
    bit_vector _restCols;

public:
    UTILITY_MOVE_DEFAULT_MEMBERS(LocalState, (_uncovered) (_support) (_supportGroups) (_restCols));

    LocalState();

    LocalState(GlobalState& global) {
        Init(global);
    }

    // Инициализировать локальное состояние
    void Init(GlobalState& g);

    AddResult AddColumn(GlobalState& g, LocalState const& l, int j);

    // Удалить столбец
    void RemoveColumn(int j);

    // Установить непокрытые столбцом j строки
    bool SetUncovered(GlobalState& g, int col, rows_citerator begin, rows_citerator end);

    bool SetUncovered(GlobalState& g, LocalState const& parent, int col);

    bool SetUncoveredSupportRows(GlobalState& global, int col, marked_ranges const& groups, rows const& support);

    bool SetUncoveredSupportRows(GlobalState& g, LocalState const& parent, int col);

    bool AddUncoveredSupportRows(GlobalState& global, int col, int l, rows_citerator begin, rows_citerator end);

    bool IsCover() const;

    // Получить набор оставшихся столбцов
    bit_mask GetRestCols() const;

    // Вернуть ненулевые столбцы
    bit_chunk CollectRestCols();

    // Получить первую непокрытую
    bit_chunk GetFrontUncovered() const;

    // Найти минимальную непокрытую строку
    bit_chunk GetMinUncovered(int mc = 0) const;

    // Удалить охватывающие строки
    void RemoveDependentRows();

    // Удалить охватывающие строки и нулевые столбцы и получить отавшиеся столбцы
    bit_chunk GetUndepCols(GlobalState& g);

    bit_chunk GetCols(GlobalState& g, AlgorithmType alg);
};

// Глобальное состояние алгоритма типа Runc
class GlobalState : public DualizationBacktrackAlgorithmBase {
private:
    friend class LocalState;

    typedef std::vector<LocalState> LocalStates;

    bit_vector _tempRow;

    LocalStates _states;

    AlgorithmType _type;

public:
    GlobalState(AlgorithmType type = AlgorithmType::RUNCM);

    void Dualize();

private:
    bool DoDualize(int stateNum);

    LocalState* GetLocalState(int stateNum);
};

}  // namespace Runc