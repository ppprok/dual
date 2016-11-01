#pragma once
#include "ECmpByRankFeatureSelector.h"
#include "BoostingLearner.h"
#include "BinaryClassifierVoter.h"


// Сбор эл.кл. через построение процедуры голосования по эл.кл.
class InitECFeatureSelector
    :public FeatureSelectorBase, public IConfigurable
{
    // Алгоритм объчения
    IClassifierLearnerPtr _learner;    

    // Собранные эл.кл.
    ECSet _ecset;

public:
    
    InitECFeatureSelector(void);    
    
    virtual void Reset();
        
    virtual void Init( Dataset const& ds );
    
    virtual void SetDefaultOptions();
    
    virtual void SetOptions( ::Options const& options );

    static void Register();

    virtual void Select( ObjectsClassRange const& cls );

    virtual void Select( ObjectsClassRange const& correct, ObjectsClassRange const& fitting );

private:

    void SetupDefaultLearner();

};

