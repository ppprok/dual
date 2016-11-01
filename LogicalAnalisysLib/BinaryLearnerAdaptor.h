#pragma once
#include "Interfaces.h"
//#include "Exceptions.h"
#include "BinaryClassifierAdaptor.h"
#include "Registrator.h"
#include "Configurable.h"


struct BinaryLearnerAdaptorOptions
{
    // Ограничение на размер класса после добавления 
    // к корректным объектам настроечных объектов
    int fittingLimit;

    BinaryLearnerAdaptorOptions()
        :fittingLimit(20)
    {
    }

};


// Обучение бинарного классификатора через обычный классификатор
class BinaryLearnerAdaptor
    :public IBinaryClassifierLearner,
    public IConfigurable
{
    IClassifierLearnerPtr _learner;

    Dataset _dataset;

public:

    BinaryLearnerAdaptorOptions Options;

    BinaryLearnerAdaptor();

    virtual IBinaryClassifierPtr Train( 
        ObjectsClassRange const& correct, 
        ObjectsClassRange const& fitting );

    virtual void Reset();

    virtual void SetOptions( ::Options const& options );

    virtual void SetDefaultOptions();

    // Регистрация фабрики объектов
    static void Register();

private:

    void SetupDefaultLearner();

};