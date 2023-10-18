#pragma once
#include "CoverCollector.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "Pruning.h"
#include "WeightedCover.h"
#include "Weighters.h"

// ���� ������������� ��������
class BitCoverCollectorCallback : public IDualizationCallback {
protected:
    bit_matrix _covers;

    Weights _weights;

public:
    virtual void Call(DualizationNode& node);

    bit_matrix const& GetCovers() const;

    Weights const& GetWeights() const;

    bit_matrix MoveCovers();

    Weights MoveWeights();

    void Reset();
};

struct MostCoverCollectorOptions {
    // ������, ������� � �������� ���������� ������ ��������
    int TrimSize;

    // ����� ����������� ��������
    int MostCount;

    // ��������� �������
    bool RandomShuffle;

    // ������� �������� (������ ���� --- �������� ���������� �� ����,
    // ������ ���� --- �������� ���������� �� ����, 0 --- �������� ���������)
    int TrimOrder;

    // ������������ ����� ��������� ����� ��������
    //int StopAfter;

    MostCoverCollectorOptions(int mostCount = 100, int trimSize = 0, int trimOrder = -1, bool randomShuffle = true) {
        assert(mostCount > 0);
        if (trimSize < mostCount)
            trimSize = mostCount * 2;

        MostCount = mostCount;
        TrimSize = trimSize;
        TrimOrder = trimOrder;
        RandomShuffle = randomShuffle;
        //StopAfter = -1;
    }
};

// ���������, ���������� N ������ ��������
class MostCoverCollector : public CoverCollector {

    int _counter;

public:
    // ����� ��������
    MostCoverCollectorOptions Options;

    // ������������ ������ ��
    WeighterBase* Weighter;

    // ���������������� ��������, �������������� ����������� ����� �� (�����������)
    MostCoverCollector(WeighterBase* weighter = 0);

    // ���������� ���� ��
    virtual void Call(DualizationNode& node);

    // ��������� �������� ��������� �������� �� ������� �� �����
    void Trim();

    // ��������� �������� ��������� ��������
    void Trim(int trimSize);
};