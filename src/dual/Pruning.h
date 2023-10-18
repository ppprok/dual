#pragma once

#include "DualizationBacktrackAlgorithmBase.h"
#include "WeightBoundsPruning.h"
#include "move_default.h"

// �������� ������ �� �� �������� �� ��������� ��� �������� �����
class CoveredRowsWeighter : public IDualizationCallback {
    // ������, ������� ���� �������
    bit_chunks _targetRows;

    typedef std::vector<int> IntVector;

    struct CoveredRow {
        int i;     // ����� ������
        int j;     // ����� �������
        Weight w;  // ��� ������
    };

    // �������� ������
    std::vector<CoveredRow> _coveredRows;

    // ��������� ������
    IntVector _uncoveredRows;

    // ���������� �������� ����� �������� �����
    WeightBoundsPruningCallback _pruner;

public:
    struct Options {
        // ������, ������� ����� �����������
        bit_chunks TargetRows;

        // ���� �����
        Weights RowsWeights;

        // ��� �� ���������
        Weight DefaultWeight;

        Options() : DefaultWeight(0) {}

        UTILITY_MOVE_DEFAULT_MEMBERS(Options, (TargetRows) (DefaultWeight) (RowsWeights));
    };

public:
    CoveredRowsWeighter();

    // �������� ��������� �� ���������
    void Reset();

    // ���������� ��� �� ���������
    void SetOptions(Options options);

    // ����������� ���� � �������� ����� � ������ �������������
    virtual void Call(DualizationNode& node);

    // ���������� ������� ������� �������
    void FixMaxBound(Weight bound);

    // ���������� ������� ������ �������
    void FixMinBound(Weight bound);

    // ������ ���������� ��������
    void TargetToMin();

    // ������ ���������� ��������
    void TargetToMax();

    // ���������� ���������� ����� �� ������� ���������
    void SetEpsilon(Weight eps);

private:
    // ��������� ��� ������ ���� ��
    void WeightNewNode(DualizationNode& node);

    // ����������� ��� ��� ����������� �� ������� �����
    void WeightBacktrackNode(DualizationNode& node);
};
