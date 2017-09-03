#pragma once
#include "PSVotingLearner.h"



class ECVotingLearner
    :public PSVotingLearner
{

        
public:

    // Подготовить и выполнить обучение классификатора,
    // Корректного на указанных объектах
    virtual IBinaryClassifierPtr Train(
        ObjectsClassRange const& correct, 
        ObjectsClassRange const& fitting);

    static void Register();

private:
    
    bool AnyExtract( Object const& o ) const;
   
    static void WeightECSet(ECSet& ecset, ObjectsRange const& objects );
};