#pragma once

#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_mask.h"
#include "bit_vector.h"

// ������� ���� ����������� ������
struct OrderByWeight {
    int order;
    Weight weight;

    OrderByWeight(int o = -1, Weight w = 0) : order(o), weight(w) {}
};

typedef std::vector<OrderByWeight> OrderByWeights;

namespace PUNC {

typedef std::vector<std::pair<int, int>> SupportRows;

// ��������� ��������� ��������� PUNC
struct LocalState {

    int topRow;

    SupportRows supportRows;

    bit_vector usedColumns;

    bit_vector restColumns;

    OrderByWeights columnsOrder;

    LocalState() : topRow(-1){};
};

typedef std::vector<LocalState> LocalStates;

// ���������� ��������� ��������� PUNC
class GlobalState : public DualizationBacktrackAlgorithmBase {

    LocalStates _states;

    bit_vector _tempRow;

    // ������� �����
    OrderByWeights _rowsOrder;

    // ������� ��������
    Weights _colsWeights;

public:
    // ���������� ������� ������ �����
    void SetRowsOrder(Weights const& weights);

    // ���������� ������� ������ ��������
    void SetColsOrder(Weights weights);

    // ��������� ������ ����������
    void Dualize();

private:
    // ��������� ���������� ������� � ���������� ������
    bool DoDualize(int level);

    // �������� ��������� ��������� �� ������
    LocalState& GetLocalState(int level);

    // ����� ������� ���������� ������
    int FindTopUncoveredRow(LocalState& state);

    // �������� ����� ������ �� �������, � ������� � ���������� �������
    int GetRowOrder(int i) const;

    // ���������������� �������� ���������
    void InitState(LocalState& state);

    // �������� ����� ���������� ��������
    bool CollectRestCols(LocalState& state);

    // ���������������� �������� ���������
    void InitChildState(LocalState& child, LocalState const& parent, int j);

    void SetOrder(Weights const& weights, int n, OrderByWeights& order);

    // ����������� �� ����������� �����
    void Sort(OrderByWeights& order);

    // ������������ ����� �������� � ��������� �������
    void CollectColsOrder(bit_chunk bits, OrderByWeights& order);
};

}  // namespace PUNC