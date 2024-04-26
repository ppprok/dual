#pragma once
#include "Object.h"
#include "CmpMatrixBuilder.h"
#include "containers.h"
#include "Pruning.h"
//#include "CoverCollectors.h"
#include "ECmpSetPredicate.h"
#include "Runc.h"

// ����� ��������� ���������� ���������� � ���������� ����������������
class ECmpSetPredicateBuilder
{
    CmpMatrixBuilder _cmpMatrixBuilder;

    ECmpSetPredicates _predicates;
    
public:

    // ���������� ��� ��������
    struct TrainInfo
    {
        // �������, ��� ������� ���������� �������� ���������� ���������
        ObjectPtrList correctObjects;
        // �������, �� ������� ������������ ����� ���� ��������
        ObjectPtrList optionalObjects;
        // �������, �� ������� ������������ �������� �� ����� �� �����
        ObjectPtrList weightedObjects;

        void Clear()
        {
            correctObjects.clear();
            weightedObjects.clear();
            optionalObjects.clear();
        }
    };

    ECmpSetPredicates& GetPredicates();

    ECmpSetPredicates const& GetPredicates() const;

    // ���������������� ��������� � ��������� �������
    void Init(
        ObjectPtrList const& positive, 
        ObjectPtrList const& negative,
        ECmpSet localBasis,
        int maxOptional = 0);

    // ����� ��������� ��������� �� ����� count ����
    void Train(
        TrainInfo const& positive, 
        TrainInfo const& negative, 
        int count);

};