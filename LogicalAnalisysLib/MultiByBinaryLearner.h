#pragma once
#include "Interfaces.h"
#include "Configurable.h"
#include "Registrator.h"


struct MultiByBinaryLearnerOptions
{
    // Часть выборки, для которой всё должно быть корректным
    float correctPart;

    // Число итераций
    int iterationCount;

    MultiByBinaryLearnerOptions()
        :iterationCount(0), correctPart(0.7f)
    {
    }

};


//////////////////////////////////////////////////////////////////////////
// Классификатор, строящий для каждого класса набор бинарных классификаторов,
// настраивая их по случайно формируемой подвыборке
//////////////////////////////////////////////////////////////////////////
class MultiByBinaryLearner
    :public IClassifierLearner,
    public IConfigurable,
    private boost::noncopyable
{

    // Метод обучения бинарных классификаторов для отделения объектов класса
    IBinaryClassifierLearnerPtr _positiveLearner;

    // Метод обучения бинарных классификаторов для отделения объектов не из класса
    IBinaryClassifierLearnerPtr _negativeLearner;

private:

    void InitFeatureSelector( Dataset const& ds );

public:

    MultiByBinaryLearnerOptions CurrentOptions;
    
    MultiByBinaryLearner();

    static void Register();

    void SetPositiveBinaryLearner(IBinaryClassifierLearnerPtr pos);
        
    void SetNegativeBinaryLearner(IBinaryClassifierLearnerPtr neg);

    virtual IClassifierPtr Train( Dataset const& ds );
    
    virtual void Reset();

    virtual void SetDefaultOptions();

    //virtual OptionsDescription const& GetOptionsDescription() const;

    virtual void SetOptions( Options const& options );
    
    //OptionsDescription CreateOptionsDescription() const;
};

