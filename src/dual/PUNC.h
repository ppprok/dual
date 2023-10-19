#pragma once

#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_mask.h"
#include "bit_vector.h"

// Задание веса порядковому номеру
struct OrderByWeight {
    int order;
    Weight weight;

    OrderByWeight(int o = -1, Weight w = 0) : order(o), weight(w) {}
};

typedef std::vector<OrderByWeight> OrderByWeights;

namespace PUNC {

typedef std::vector<std::pair<int, int>> SupportRows;

// Локальное состояние алгоритма PUNC
struct LocalState {

    int topRow;

    SupportRows supportRows;

    bit_vector usedColumns;

    bit_vector restColumns;

    OrderByWeights columnsOrder;

    LocalState() : topRow(-1){};
};

typedef std::vector<LocalState> LocalStates;

// Глобальное состояние алгоритма PUNC
class GlobalState : public DualizationBacktrackAlgorithmBase {

    LocalStates _states;

    bit_vector _tempRow;

    // Порядок строк
    OrderByWeights _rowsOrder;

    // Порядок столбцов
    Weights _colsWeights;

public:
    // Установить порядок обхода строк
    void SetRowsOrder(Weights const& weights);

    // Установить порядок обхода столбцов
    void SetColsOrder(Weights weights);

    // Выполнить полную дуализацию
    void Dualize();

private:
    // Выполнить дуализацию начиная с указанного уровня
    bool DoDualize(int level);

    // Получить локальное состояние по уровню
    LocalState& GetLocalState(int level);

    // Найти верхнюю непокрытую строку
    int FindTopUncoveredRow(LocalState& state);

    // Получить номер строки по порядку, в котором её необходимо покрыть
    int GetRowOrder(int i) const;

    // Инициализировать корневое состояние
    void InitState(LocalState& state);

    // Получить набор непокрытых столбцов
    bool CollectRestCols(LocalState& state);

    // Инициализировать дочернее состояние
    void InitChildState(LocalState& child, LocalState const& parent, int j);

    void SetOrder(Weights const& weights, int n, OrderByWeights& order);

    // Упорядочить по возрастанию весов
    void Sort(OrderByWeights& order);

    // Сформировать набор столбцов в требуемом порядке
    void CollectColsOrder(bit_chunk bits, OrderByWeights& order);
};

}  // namespace PUNC