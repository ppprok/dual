#pragma once

#include "Interfaces.h"
#include "BinaryClassifierNormalizer.h"


typedef std::vector<IBinaryClassifierPtr> BinaryClassifiers;

typedef std::map<int, BinaryClassifiers> BinaryClassifiersMap;

// Голосование по бинарным классификаторам, связанных с определёнными метками классов
class BinaryClassifierVoter
    : public ClassifierBase, public IECVoting
{

    // Классификаторы, сгруппированные по классам
    BinaryClassifiersMap _classifiers;

public:

    BinaryClassifiersMap const& GetClassifiers() const 
    { 
        return _classifiers; 
    }

    // Инициализировать пустые наборы классификаторов для каждого класса
    void Init(Dataset const& ds, int reserve = 0);

    // Добавить классификатор
    BinaryClassifierNormalizerPtr AddClassifier(int label, IBinaryClassifierPtr classifier, Weight w = 1);

    // Добавить классификатор и обновить оценки за отнесение прецедентов к классам
    BinaryClassifierNormalizerPtr AddClassifierAndUpdateEstimates( 
        int label, 
        IBinaryClassifierPtr cls, 
        ObjectPtrList const &objects, 
        Predictions& predictions,
        Weight w = 1);
   

    // Посчитать голоса для объекта
    virtual Prediction Apply( Object const& o ) const;
    
    virtual void GetECSet( ECSet& ecset );
};
