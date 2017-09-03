#pragma once
#include "Interfaces.h"
#include "FeatureSelectorBase.h"
#include "Registrator.h"
#include "ECVotingLearner.h"


// ќтбор признаков бинарным классификатором
class BinaryECFeatureSelector
    : 
    public FeatureSelectorBase, 
    public IConfigurable
{

    // Ѕинарный классификатор, осуществл€ющий выбор признаков
    IBinaryClassifierLearnerPtr _learner;

    ECSet _ecset;

public:

    BinaryECFeatureSelector();

    void SetDefaultOptions();

    void SetOptions( ::Options const& options );
    
    void Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting );

    virtual void Select( ObjectsClassRange const& correct );

    virtual void Reset();

    static void Register()
    {
        Registrator<IFeatureSelectorPtr>::Instance().
            RegisterFactory("BinaryECFeatureSelector", [](){return std::make_shared<BinaryECFeatureSelector>();});
    }

    virtual void Init( Dataset const& ds );

private:

    void CollectECmpSet(ObjectsClassRange const& correct);
    
    void SetupDefaultLearner();    
    
};