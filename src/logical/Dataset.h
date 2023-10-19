#pragma once
#include "Object.h"
#include "ObjectsClass.h"
#include "Predictions.h"

struct ILearnableClassifier;

typedef std::vector<int> Labels;

// ����� �����������
struct Dataset
{
    // ��� �������
	ObjectPtrList objects;

	// ������ ��������
    ObjectsClasses classes;

    // �������� ���������, ������������� � ��������
    FeatureValues featureValues;

    // ���������� ���� ��������
    Weights weights;

    // ����������� ����� ��������
    Labels labels;

    Dataset();

    //UTILITY_MOVE_DEFAULT_MEMBERS(Dataset, (objects)(classes)(featureValues));
    
    // ������� ���������� �� �������� � �������
    ObjectsClasses const& CollectClasses();

    // ������������ ��������--��������
    FeatureValues const& CollectFeatureValues();
    
    // �������� ��������--�������� �������� ��������� �������
    FeatureValues const& AddFeatureValues(ObjectsClasses const& classes);

    // �������� ��������--�������� ��������� ��������
    FeatureValues const& AddFeatureValues(ObjectPtrList const& objects);

	// ��������� �� �����
    bool Load(std::string const& filename);

	// ������������ ������� ��������� �������
	void RandomShuffle();
    
	    
    // �������� ���� �������--�������� ��� �������
    void AddFeatureValues(Object const& o);

    // ����� ����� �� �����
    ObjectsClass const& GetClass(int label) const;

    // ����������� ������� �� ����
    void OrderByWeight(ObjectPtrList& objects) const;

	// ����������� ������� �� �������� ����
	void OrderByWeightDescent(ObjectPtrList& objects) const;
    
    void Clear();
   
    static void TakeRange( ObjectPtrList const& objects, float from, float to, ObjectPtrList& output );
    
    static void TakeRange( ObjectsClasses const& classes, float from, float to, ObjectPtrList& output );
    
    void TakeRange( float from, float to, ObjectPtrList& output );
    
    std::pair<Dataset, Dataset> Split(float p1, float p2 ) const;

    Weights& StoreWeights();

    void RestoreWeights(Weights const& weights );

    void RestoreWeights();

    // ��������� ����� ��������
    Labels& StoreLabels();

    // ������������ ����� ��������
    void RestoreLabels(Labels const& labels );

    
    // ������������ ����� ��������
    void RestoreLabels();

    // �������������� ����� �������� � ������� �����������
    template <typename Map>
    void RemapLabels(Map map)
    {
        for (auto& o : objects)
            o->label = map(*o);
    }

    // ������� �������, � ������� ���������� ��������, �� ������ �����
    bool RemoveUncorrect();


};



