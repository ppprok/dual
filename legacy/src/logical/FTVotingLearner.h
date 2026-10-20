#pragma once

#include "bit_chunk.h"
#include "Dataset.h"
#include "Object.h"
#include "Interfaces.h"
#include "Configurable.h"
#include "GeneticDualizer.h"
#include "Registrator.h"
#include "FTVoting.h"
#include "CmpMatrix.h"
#include "CmpFitMatrixBuilder.h"


// Метод обучения тестового алгоритма
class FTVotingLearner
    :
    public IBinaryClassifierLearner,
    public IConfigurable,
    public INeedFeatureSelector
{
protected:
    
    // Выбор признаков
    IFeatureSelectorPtr _featureSelector;

    // Посроение матрицы сравнения и её покрытий
    CmpFitMatrixBuilder _cmpFitMatrixBuilder;

    // Сравнение признаков
    std::shared_ptr<FeatureComparer> _featureComparer;

    // Временный массив для хранения отобранных признаков
    Features _features;

    // Сравнение по эл.кл.
    std::shared_ptr<ECmpSetComparer> _ecmpSetComparer;

    // Временный массив для хранения отобранных эл.кл. с отношениями
    ECmpSet _ecmpSet;
        
public:
        
    FTVotingLearner();

    static void Register();

    virtual void SetDefaultOptions();
        
    virtual void SetOptions( Options const& options );
    
    virtual void Reset();

    virtual IBinaryClassifierPtr Train( ObjectsClassRange const& correct, ObjectsClassRange const& fitting);

    IBinaryClassifierPtr DoTrainECmp( ObjectsClassRange const &correct, ObjectsClassRange const& fitting );
    
    IBinaryClassifierPtr DoTrainFT( ObjectsClassRange const &correct, ObjectsClassRange const& fitting );
    
    virtual void SetFeatureSelector( IFeatureSelectorPtr selector );

    virtual IFeatureSelectorPtr GetFeatureSelector();

    // Получить доступ к опция построения матрицы сравнения и её покрытий
    CmpFitMatrixBuilderOptions& GetOptions()
    {
        return _cmpFitMatrixBuilder.Options;
    }

private:
    
    IBinaryClassifierPtr DoTrain( ObjectsClassRange const& correct, ObjectsClassRange const& fitting);

protected:

    void InitFeatureComparer();
    
    void InitECmpSetComparer();
    
};

