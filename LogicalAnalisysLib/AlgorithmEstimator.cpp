#include "stdafx.h"
#include "AlgorithmEstimator.h"
#include "Registrator.h"
//#include "Exceptions.h"


AlgorithmEstimator::AlgorithmEstimator(void)
{
    SetDefaultOptions();
}


GroupedPredictions AlgorithmEstimator::
    FoldsCrossValidation( Dataset ds, IClassifierLearnerPtr a, float trainPart /*= 0.9*/, int count /*=1*/ )
{
    int folds = (int)ceil(1 / (1-trainPart));

    LOG_SEV(info) << "CV, folds="<< folds << "; time=" << count;
    
    GroupedPredictions gp;
    if (folds <= 0 || count <= 0)
        return gp;

    gp.groups.reserve(count);

    ObjectPtrList objects = ds.objects;
    Dataset train;
    Dataset test;

    float r = 1;
    r /= folds;

    if (CurrentOptions.randomSeed)
        srand(CurrentOptions.randomSeed);
    
    // Все случайные разбиения зависят от состояния генератора в текущий момент времени и 
    // не зависят от использования генератора при обучении
    blob<int> seeds(count * folds);
    for (int t = 0; t < count; ++t)
        seeds[t] = rand();

    int s = 0;

    for (int t = 0; t < count; ++t)
    {
        for (int f = 0; f < folds; ++f)
        {
            srand(seeds[s++]);
            if (f == 0)
                ds.RandomShuffle();
                        
            train.Clear();
            test.Clear();

            ds.TakeRange(f*r, (f+1)*r, test.objects);
            ds.TakeRange(0, f*r, train.objects);
            ds.TakeRange((f+1)*r, 1.0, train.objects);

            train.CollectClasses();
            //train.CollectFeatureValues();
            
            a->Reset();
            auto clsr = a->Train(train);
            
            GroupedPredictions pp(clsr->ApplyMany(test.objects));
            
            LOG_SEV(info) << "CV iteration complete: f=" <<f << "; t=" << t << " ; err=" << pp.GetMeanError();
            
            gp.groups.emplace_back(std::move(pp.groups.back()));            
        }
    }

    return std::move(gp);
}



GroupedPredictions AlgorithmEstimator::LeaveOneOutCrossValidation( Dataset ds, IClassifierLearnerPtr a )
{
    int m = ds.objects.size();
    
    LOG_SEV(info) << "LOO: m=" << m;
        
    if (CurrentOptions.randomSeed)
        srand(CurrentOptions.randomSeed);

    // Все случайные разбиения зависят от состояния генератора в текущий момент времени и 
    // не зависят от использования генератора при обучении
    blob<int> seeds(m);
    for (int t = 0; t < m; ++t)
        seeds[t] = rand();
        
    Predictions pp;
    pp.reserve(m);

    for (int i = 0; i < m; ++i)
    {
        srand(seeds[i]);
        
        std::swap(ds.objects[i], ds.objects.back());
        ObjectPtr test = ds.objects.back();
        ds.objects.pop_back();
        ds.CollectClasses();
        //ds.CollectFeatureValues();

        a->Reset();
        pp.push_back(a->Train(ds)->Apply(*test));        

        LOG_SEV(trace) << "LOO iteration " << i << " label=" << pp.back().label << " prediction=" << pp.back().LabelByEstimates();

        ds.objects.push_back(test);
        std::swap(ds.objects[i], ds.objects.back());
    }

    return GroupedPredictions(std::move(pp));
}

GroupedPredictions AlgorithmEstimator::RandomSplitCrossValidation(
    Dataset ds, IClassifierLearnerPtr a, float trainPart, int count /*=1*/ )
{
    GroupedPredictions gp;
    if (count <= 0)
        return gp;

    gp.groups.reserve(count);

    ObjectPtrList objects = ds.objects;
    Dataset train;
    Dataset test;

    if (CurrentOptions.randomSeed)
        srand(CurrentOptions.randomSeed);

        
    // Все случайные разбиения зависят от состояния генератора в текущий момент времени и 
    // не зависят от использования генератора при обучении
    blob<int> seeds(count);
    for (int t = 0; t < count; ++t)
        seeds[t] = rand();

    for (int t = 0; t < count; ++t)
    {
        srand(seeds[t]);
        ds.RandomShuffle();

        train.Clear();
        test.Clear();

        ds.TakeRange(trainPart, 1, test.objects);
        ds.TakeRange(0, trainPart, train.objects);
        
        train.CollectClasses();
        train.CollectFeatureValues();

        a->Reset();
        gp.groups.push_back(a->Train(train)->ApplyMany(test.objects));
    }

    return std::move(gp);
}

void AlgorithmEstimator::SetDefaultOptions()
{
    CurrentOptions = AlgorithmEstimatorOptions();
    TrySetDefaultOptions(algorithm);
}


void AlgorithmEstimator::SetOptions( Options const& options )
{
    
    auto ds = options.get("trainDataset", "");
    if (! ds.empty())
    {
        LOG_SEV(info) << "Dataset: " << ds;
        trainDataset.Load(ds);
    }
    
    ds = options.get("testDataset", "");
    if (! ds.empty())
    {
        LOG_SEV(info) << "Test dataset: " << ds;
        testDataset.Load(ds);
    }
       

    AlgorithmEstimatorOptions def;
    
    CurrentOptions.validationMethod = options.get<std::string>("method", def.validationMethod);
    LOG_SEV(info) << "Validation method: " << CurrentOptions.validationMethod;

    CurrentOptions.randomSeed = options.get<int>("random-seed", def.randomSeed);
    CurrentOptions.times = options.get<int>("times", def.times);
    CurrentOptions.trainPart = options.get<float>("trainPart", def.trainPart);
    
    std::string alg = options.get<std::string>("algorithm.type");
    LOG_SEV(info) << "Algorithm: " << alg;
    
    algorithm = TryCreateChild<IClassifierLearnerPtr>(options, "algorithm");

}

Predictions AlgorithmEstimator::TestCorrectness( Dataset const& ds, IClassifierLearnerPtr a )
{
    if (CurrentOptions.randomSeed)
        srand(CurrentOptions.randomSeed);
    
    return a->Train(ds)->ApplyMany(ds.objects);
}

Predictions AlgorithmEstimator::ValidateByTestDataset( Dataset const& train, Dataset const& test, IClassifierLearnerPtr a )
{
    if (CurrentOptions.randomSeed)
        srand(CurrentOptions.randomSeed);
    
    return a->Train(train)->ApplyMany(test.objects);
}

void AlgorithmEstimator::Run()
{
    assert(algorithm);
    
        
/*    if (CurrentOptions.randomSeed)
    {
        srand(CurrentOptions.randomSeed);
    }
    else
    {
        srand((int)time(0));
    }*/

    if (CurrentOptions.validationMethod == "CV")
    {
        result = FoldsCrossValidation(trainDataset, algorithm,  CurrentOptions.trainPart, CurrentOptions.times);
        LOG_SEV(info) << "Mean error=" << result.GetMeanError();
        return;
    }

    if (CurrentOptions.validationMethod == "RS")
    {
        result = RandomSplitCrossValidation(trainDataset, algorithm,  CurrentOptions.trainPart, CurrentOptions.times);
        LOG_SEV(info) << "Mean error=" << result.GetMeanError();
        return;
    }

    if (CurrentOptions.validationMethod == "LOO")
    {
        result = LeaveOneOutCrossValidation(trainDataset, algorithm);
        LOG_SEV(info) << "Mean error=" << result.GetMeanError();
        return;
    }

    if (CurrentOptions.validationMethod == "T")
    {
        result = ValidateByTestDataset(trainDataset, testDataset, algorithm);
        LOG_SEV(info) << "Mean error=" << result.GetMeanError();
        return;
    }
    
    if (CurrentOptions.validationMethod == "C")
    {
        result = TestCorrectness(trainDataset, algorithm);
        LOG_SEV(info) << "Mean error=" << result.GetMeanError();
        return;
    }
}

/*OptionsDescription AlgorithmEstimator::CreateOptionsDescription() const
{
    OptionsDescription desc;
    
    desc.add_options()
        ("help", "produce help message")
        ("help-algorithm", "produce help message for algorithm")
        ("dataset", po::value<std::string>()->default_value(""), "input file name for dataset (one line per object; each object defined by label and feature values)")
        ("test-dataset", po::value<std::string>()->default_value(""), "input file name for test dataset")
        ("method", po::value<std::string>()->default_value("CV"), "validation method: CV - cross-validation, S - split, LOO - leave-one-out, T --- by test dataset, C --- test correctness")
        ("times", po::value<int>()->default_value(5), "split times")
        ("train-part", po::value<float>()->default_value(0.9f), "part of training data for split")
        ("random-seed", po::value<int>()->default_value(777), "random seed before split data")
        ("algorithm", po::value<std::string>()->default_value("Boosting"),  
        "algorithm: Boosting, MultiByBinary");
        ("algorithm-config", po::value<std::string>()->default_value("Boosting"),  
        "algorithm: Boosting, MultiByBinary");

    return desc;
}*/