#pragma once

#include "DualizationBacktrackAlgorithmBase.h"

// �������� ������ �� �� �������� �� ��������� ��� ��������
class WeightBoundsPruningCallback : public IDualizationCallback {
protected:
    // ���� ��������
    Weights _weights;

    // ����������� ���������� ���
    Weight _minWeight;

    // ������������ ���������� ���
    Weight _maxWeight;

    // ����� ������������� �����
    Weight _restPositive;

    // ����� ������������� �����
    Weight _restNegative;

    // ����������� �������� ������ ������� �� (������������ ��� - _epsilon)
    bool _dynamicMinBound;

    // ����������� �������� ������� ������� �� (����������� ��� + _epsilon)
    bool _dynamicMaxBound;

    // ������ ������������ ������� �� ������������ (�������������) ���������� ����
    Weight _epsilon;

    // ��� ��������, ��� ������� �� �� ����� � ������� _weights;
    Weight _defaultWeight;

public:
    // ���������� ��������
    void PrunCoverNode(DualizationNode& node);

    WeightBoundsPruningCallback();

    // �������� ��������� �� ���������
    void Reset();

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

    // ���������� ��� �� ���������
    void SetDefaultWeight(Weight w);

    // ���������� ����
    void SetWeights(Weights weights);

    // �������� ��� ������� j
    Weight GetWeight(int j) const;

    bool IsDefaultWeight(int j) const;

    // ����������� ���� � �������� ����� � ������ �������������
    virtual void Call(DualizationNode& node);

    void DecRestWeight(Weight w);
    void IncRestWeight(Weight w);
    void PrunInnerNode(DualizationNode& node);
};
