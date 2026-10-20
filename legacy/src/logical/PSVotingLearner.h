#pragma once
#include "Interfaces.h"
#include "FTVotingLearner.h"

// Голосование по представительным наборам
class PSVotingLearner
    : public FTVotingLearner
{
protected:

    // Временный массив для хранения представительных наборов
    ECSet _ecset;
    
    // Временный массив для хранения отобранных исключенных объектов
    ObjectPtrList _excluded;
        
    // Объединить одинаковые эл.кл. и проссумировать их веса
    void MergeCollectedECSet();

    // Собрать представительные наборы
    void CollectECSet(ObjectsRange const& objects);

    // Собрать представительные наборы
    void CollectECSet(ObjectsRange const& objects, OptionalObjects& oo, ComparersComposer& cc);

public:

    virtual IBinaryClassifierPtr Train(ObjectsClassRange const& correct, ObjectsClassRange const& fitting);
        
    static void Register();    
};