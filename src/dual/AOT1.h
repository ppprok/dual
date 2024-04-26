#pragma once

#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_chunk.h"
#include "marked.h"

namespace AOT1 {

//typedef raw_vector<marked<range>> marked_ranges;
typedef std::vector<bit_chunk> rows;
typedef rows::iterator rows_iterator;
typedef rows::const_iterator rows_citerator;

enum AlgorithmType {
    AO1,
    AO2,
    AO1K,
    AO2K,
    AO1M,
    AO2M
};

class GlobalState;

// ��������� ��������� ��������� ���� AOT1
class LocalState {

    friend class GlobalState;

    enum AddResult {
        Inner,
        Cover,
        Uncompatable,
        Uncoverable
    };

private:
    // ���������� ������
    rows _uncovered;

    // ������������ ������
    rows _competing;

    // ���������� �������
    bit_chunk _restCols;

public:
    LocalState(){};

    LocalState(LocalState&& s);

    LocalState& operator=(LocalState&& s);

    // ���������������� ��������� ���������
    void Init(GlobalState& g);

    AddResult AddItem(GlobalState& g, LocalState const& l, int i, int j);

    void RemCol(int j);

    bool IsCover() const;

    // �������� ����� ���������� ��������
    bit_chunk GetRestCols() const;

    // ������� ��������� �������
    bit_chunk CollectRestCols();

    // �������� ������ ����������
    bit_chunk GetFrontUncovered() const;

    // ����� ����������� ���������� ������
    bit_chunk GetMinUncovered(int mc = 0) const;

    // ������� ������������ ������
    void RemoveDependentRows();

    // ������� ������������ ������ � ������� ������� � �������� ���������� �������
    bit_chunk GetNonZeroCols(GlobalState& g);

    bit_chunk GetCols(GlobalState& g, AlgorithmType alg);
};

// ���������� ��������� ��������� ���� AOT1
class GlobalState : public DualizationBacktrackAlgorithmBase {
private:
    friend class LocalState;

    typedef std::vector<LocalState> LocalStates;

    bit_matrix _tempMatrix;

    bit_chunk _tempRow;

    LocalStates _states;

    AlgorithmType _type;

public:
    GlobalState(AlgorithmType type = AlgorithmType::AO1M) : _type(type) {}

    void Dualize();

private:
    bool DoDualize(int stateNum);

    LocalState* GetLocalState(int stateNum);
};

}  // namespace AOT1