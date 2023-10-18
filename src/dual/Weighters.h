#pragma once

#include <vector>

#include "DualizationBacktrackAlgorithmBase.h"
#include "bit_matrix.h"

//////////////////////////////////////////////////////////////////////////
// ������� ����� ��� ����������� ����� ��
//////////////////////////////////////////////////////////////////////////
class WeighterBase : virtual public IDualizationCallback {
protected:
    // ����� ���������� ������������� �����
    Weight _restPositive;

    // ����� ���������� ������������� �����
    Weight _restNegative;

    // ����������� ���������� ���
    Weight _minWeight;

    // ������������ ���������� ���
    Weight _maxWeight;

    // ����
    Weights _weights;

    // ������������ ����� ��������� � �����
    int _weightSumCount;

    // ������� ����������������� ������ ���� ���������� ������������� � ������������� ���������
    void InvalidateRestWeights();

    // ���������, ������������� �� ������ ���� ���������� ������������� � ������������� ���������
    bool IsRestWeightsValid() const;

    // ��������� ������ ���� ���������� ������������� � ������������� ���������
    void CalculateRestWeights();

    // ��������� ����� ���������� ����
    void IncRestWeight(Weight w);

    // ��������� ����� ���������� ����
    void DecRestWeight(Weight w);

public:
    WeighterBase();

    void WeightInnerNode(DualizationNode& node);

    void SetWeights(Weights weights);

    Weight GetWeight(int i);

    Weight GetDefaultWeight();

    void Reset();

    void SetMinWeight(Weight w);

    void SetMaxWeight(Weight w);

    Weight GetRestPositive() const;

    Weight GetRestNegative() const;

    // ������ ����� �� �� ���������� ��������� �������� ����
    bool Prun(Weight current, Weight minWeight, Weight maxWeight) const;

    // ������ ����� �� �� ���������� ��������� ���� ����
    bool Prun(DualizationNode& node, Weight minWeight, Weight maxWeight) const;

    // ������ ����� �� �� ������� ���������� ��������� ���� ����
    bool Prun(DualizationNode& node);

    virtual void WeightNewNode(DualizationNode& node) = 0;

    virtual void WeightBacktrackNode(DualizationNode& node) = 0;

    void Call(DualizationNode& node);
};

// ����������� ����� �� �� ���������� ���� ��������
class ColumnsWeighterCallback : public WeighterBase {

    // ��������� ��� ������ ���� ��
    void WeightNewNode(DualizationNode& node);

    // ����������� ��� ��� ����������� �� ������� �����
    void WeightBacktrackNode(DualizationNode& node);
};

// ����������� ����� �� �� ���������� ���� �������� � ���������� �����
class CoveredRowWeighterCallback : public WeighterBase {
    // ������, ������� ���� �������
    bit_chunks _targetRows;

    typedef std::vector<int> IntVector;

    struct CoveredRow {
        int i;     // ����� ������
        int j;     // ����� �������
        Weight w;  // ��� ������
    };

    // ����������� ������
    std::vector<CoveredRow> _coveredRows;

    // ��������� ������
    IntVector _uncoveredRows;

public:
    CoveredRowWeighterCallback();

    // �������� ��������� �� ���������
    void Reset();

    // ���������� ������ ��� ��������
    void SetTargetRows(bit_chunks targetRows);

    // ��������� ��� ������ ���� ��
    void WeightNewNode(DualizationNode& node);

    // ����������� ��� ��� ����������� �� ������� �����
    void WeightBacktrackNode(DualizationNode& node);
};