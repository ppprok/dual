#pragma once
#include "Object.h"
#include "CmpMatrixBuilder.h"
#include "containers.h"
#include "ECmpVoting.h"
#include "Interfaces.h"
#include "Configurable.h"
#include "GeneticDualizer.h"
#include "Registrator.h"


struct ECmpVotingLearnerOptions
{
    int ecmpCount;
    
    int optionalCount;
    
    int fittingLimit;
    
    ECmpVotingLearnerOptions()
    {
        ecmpCount = 1;
        optionalCount = 5;
        fittingLimit = 100;
    }
};

//////////////////////////////////////////////////////////////////////////
// Алгоритм голосования по предикатам вида $B_{(U,O,G)}$
//////////////////////////////////////////////////////////////////////////
class ECmpVotingLearner
    :public IBinaryClassifierLearner, 
    public IConfigurable,
    public INeedFeatureSelector,
    private boost::noncopyable
{
    
    GeneticDualizer _geneticAlgorithm;

    //CoveredRowsWeightTargetFunc _geneticTarget;

    IFeatureSelectorPtr _featureSelector;

    ObjectPtrList _fittingObjects;

    ObjectPtrList _optionalObjects;

    ObjectsClass _correct;

    ObjectsClass _fitting;

    ECmpSet _localBasis;

public:

    ECmpVotingLearnerOptions CurrentOptions;
        
    static void Register();

    ECmpVotingLearner();

    // Обучить бинарный алгоритм
    virtual IBinaryClassifierPtr Train( ObjectsClass const& cls, InformativityCondition const& informativity );

    // Обучить бинарный алгоритм
    virtual IBinaryClassifierPtr Train( ObjectsClass const& correct, ObjectsClass const& fitting);

    // Сбросить все построенные предикаты
    virtual void Reset();
    
    // Получить допустимые опции
    //virtual OptionsDescription const& GetOptionsDescription() const;

    // Установить опции обучения
    virtual void SetOptions(Options const& options);
    
    // Установить селектор признаков
    virtual void SetFeatureSelector( IFeatureSelectorPtr selector );

    // Получить текущий селектор признаков
    virtual IFeatureSelectorPtr GetFeatureSelector();
    
    // Создать описание опций алгоритма
    //OptionsDescription CreateOptionsDescription() const;

    virtual void SetDefaultOptions();

private:
    
    // Найти генетическим алгоритмом лучшие предикаты
    ECmpSetInfos BuildECmpSets( 
        ObjectsRange positive, 
        ObjectsRange optional,
        ObjectsRange negative,
        ObjectsRange fitting,
        ECmpSet localBasis, 
        int count);
    
    // Построить матрицу сравнения
    bit_matrix BuildCmpMatrix( 
        ObjectsRange positive, 
        ObjectsRange optional, 
        ObjectsRange negative,
        ECmpSet const& localBasis, 
        Weights& weights);
    
    void CollectFittingObjects( ObjectsClass const& fitting );
    
    void SelectFeatures( ObjectsClass const& correct );
};