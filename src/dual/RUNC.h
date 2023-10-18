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

// ��������� ��������� ��������� ���� Runc
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
    // ���������� ������
    rows _uncovered;

    // ������� ������
    rows _support;

    // ������ ������� ��������
    marked_ranges _supportGroups;

    // ���������� �������
    bit_vector _restCols;

public:
    UTILITY_MOVE_DEFAULT_MEMBERS(LocalState, (_uncovered) (_support) (_supportGroups) (_restCols));

    LocalState();

    LocalState(GlobalState& global) {
        Init(global);
    }

    // ���������������� ��������� ���������
    void Init(GlobalState& g);

    AddResult AddColumn(GlobalState& g, LocalState const& l, int j);

    // ������� �������
    void RemoveColumn(int j);

    // ���������� ���������� �������� j ������
    bool SetUncovered(GlobalState& g, int col, rows_citerator begin, rows_citerator end);

    bool SetUncovered(GlobalState& g, LocalState const& parent, int col);

    bool SetUncoveredSupportRows(GlobalState& global, int col, marked_ranges const& groups, rows const& support);

    bool SetUncoveredSupportRows(GlobalState& g, LocalState const& parent, int col);

    bool AddUncoveredSupportRows(GlobalState& global, int col, int l, rows_citerator begin, rows_citerator end);

    bool IsCover() const;

    // �������� ����� ���������� ��������
    bit_mask GetRestCols() const;

    // ������� ��������� �������
    bit_chunk CollectRestCols();

    // �������� ������ ����������
    bit_chunk GetFrontUncovered() const;

    // ����� ����������� ���������� ������
    bit_chunk GetMinUncovered(int mc = 0) const;

    // ������� ������������ ������
    void RemoveDependentRows();

    // ������� ������������ ������ � ������� ������� � �������� ��������� �������
    bit_chunk GetUndepCols(GlobalState& g);

    bit_chunk GetCols(GlobalState& g, AlgorithmType alg);
};

// ���������� ��������� ��������� ���� Runc
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