#include "stdafx.h"

#include "logging_utils.h"
#include "BoostingLearner.h"
#include "math_utils.h"
#include "Interfaces.h"
#include "BinaryClassifierVoter.h"
//#include "Exceptions.h"
#include "ECmpByRankFeatureSelector.h"
#include "BinaryClassifierBoolAdaptor.h"
#include "BinaryClassifierNormalizer.h"
#include "ECVotingLearner.h"






BoostingOptions::BoostingOptions()
{
    iterationsCount = 0;
    stopQ = 1;
    minWeight = 0.05f;
    positivePart = 0;
    positiveCount = 5;
}



void BoostingLearner::InitFeatureSelector( Dataset const& ds )
{
    auto nfs = dynamic_cast<INeedFeatureSelector*>(positiveLearner.get());
    IFeatureSelectorPtr fs1;
    if (nfs)
        fs1 = nfs->GetFeatureSelector();

    nfs = dynamic_cast<INeedFeatureSelector*>(negativeLearner.get());
    IFeatureSelectorPtr fs2;
    if (nfs)
        fs2 = nfs->GetFeatureSelector();

    if (fs2 == fs1)
        fs2 = 0;

    if (fs1)
    {
        fs1->Reset();
        fs1->Init(ds);
    }

    if (fs2)
    {
        fs2->Reset();
        fs2->Init(ds);
    }
}


IClassifierPtr BoostingLearner::Train(Dataset const& ds)
{
    BOOST_LOG_NAMED_SCOPE("BoostingLearner::Train");

    assert (positiveLearner || negativeLearner);
    
    Dataset& dsref = const_cast<Dataset&>(ds);    
    auto weights = std::move(dsref.StoreWeights());
        
    InitFeatureSelector(ds);
    
    auto voter = std::make_shared<BinaryClassifierVoter>();
    voter->Init(ds);
    auto predictions = voter->ApplyMany(ds.objects);
    
    int m = ds.objects.size();
    int l = ds.classes.size();    
            	
    int T = CurrentOptions.iterationsCount;
    if (T <= 0)
        T = CalculateIterationCount(ds);
    Weight delta = log((Weight)m) / T;

    LOG_SEV(trace) << "Iteration count=" << T << "; Delta=" << delta;
            
    for (int t = 0; t < T; ++t)
    {
        CollectClassWeightInfo(ds.classes, predictions, delta);

        auto clsinfo = SelectClass(ds, predictions, delta);
                                
        if (clsinfo.Q < CurrentOptions.stopQ)
			break;

        WeightObjects(ds, predictions, clsinfo);

        auto& clsref = const_cast<ObjectsClass&>(ds.GetClass(clsinfo.label));

        bool complement = false;
                
        /*if (positiveLearner && negativeLearner)
        {
            complement = rand()%2 == 1;            
        }
        else if (negativeLearner)
        {
            complement = true;
        }

                
        if (complement)
        {
            clsinfo = clsinfo.GetComplement();*/
        
        //ObjectsClassRange correctPart(clsref.label);
        //ObjectsClassRange fittingPart(clsref.label);
        
        /*int q = GetCorrectPositiveObjectsCount(clsref.positive.size());
        
        clsref.SortObjectsByWeights();
        clsref.SplitOptimal(clsinfo.informativity, correctPart, fittingPart, q);*/

        CollectCorrectObjects(clsref, clsinfo);
        
        auto weakLearner = complement 
            ? negativeLearner 
            : positiveLearner;

        weakLearner->Reset();        
        auto bincls = weakLearner->Train(_correctPart, _fittingPart);
        
        auto normcls = std::make_shared<BinaryClassifierNormalizer>(bincls);
        normcls->Normalize(ds.objects);
        
        //bincls = std::make_shared<BinaryClassifierBoolAdaptor>(bincls);
                
        Weight w = WeightBinaryClassifier(clsinfo, normcls, clsref.positive, clsref.negative);

        LOG_SEV(trace) << 
            "t=" << t << 
            "; label=" << clsinfo.label << 
            "; Q=" << clsinfo.Q << 
            "; delta=" << delta << 
            "; w=" << w;

        if (w < 0)
            continue;

        if (w < CurrentOptions.minWeight)
            w = CurrentOptions.minWeight;
        
        /*if (complement)
        {
            clsref.MakeComplement();
            w = -w;
        }*/

        voter->AddClassifierAndUpdateEstimates(clsinfo.label, normcls, ds.objects, predictions, w);
    }

    dsref.RestoreWeights(weights);

    return voter;
}

void BoostingLearner::Reset()
{    
    if (negativeLearner)
        negativeLearner->Reset();

    if (positiveLearner)
        positiveLearner->Reset();
}

// Сбор информации о суммарных весах объектов в классах и их дополнениях
void BoostingLearner::CollectClassWeightInfo(
        ObjectsClasses const& classes, 
        Predictions const& predictions, 
        Weight delta)
{
    _weightedClasses.clear();
    int l = classes.size();
    /*if (positiveLearner)
        l += classes.size();
    if (negativeLearner)
        l += classes.size();*/
    
    if (! l)
        return;
    
    _weightedClasses.reserve(l);

    for(auto& c:classes)
        _weightedClasses.emplace_back(c.label);
    
    l = classes.size();
    Weight Q = 0;
	
    for (auto& prediction : predictions)
    {
        int labelPos = prediction.PosByLabel(prediction.label);
		assert(inside(_weightedClasses, labelPos) && _weightedClasses[labelPos].label == prediction.label);

		Weight labelEst = prediction.predictions[labelPos].estimate;
		
        for (int i = 0; i < l; ++i)
        {
			assert(prediction.predictions[i].label == _weightedClasses[i].label);
			
			if (_weightedClasses[i].label == prediction.label)
				continue;			
                
			Weight w = exp(-labelEst + prediction.predictions[i].estimate);

			_weightedClasses[i].AddNegativeWeight(w);
			_weightedClasses[labelPos].AddPositiveWeight(w);			
            Q += w;
        }        
    }
    
    
    /*if (negativeLearner && positiveLearner)
    {
        for (int i = 0; i < l; ++i)
        {
            _weightedClasses.push_back(_weightedClasses[i].GetComplement());
        }
    }
    else if (negativeLearner)
    {
        for (int i = 0; i < l; ++i)
        {
            _weightedClasses[i] = _weightedClasses[i].GetComplement();
        }
    }*/

    auto th = sqrt(delta * Q);
    for (auto& i:_weightedClasses)
    {
        i.Q = Q;
        i.informativity.threshold = th;
        //i.informativity.minN /= 2;
        //i.informativity.minP += i.informativity.minN;
    }    

}


// Выбор класса, для которого будет осуществляться обучение бинарного классификатора
BoostingClassInfo 
    BoostingLearner::SelectClass(
    Dataset const &ds, 
    Predictions const& predictions, 
    Weight delta)
{
    
    _selectedClasses.clear();

    auto getWeight1 = [this](int i)
                        {return _weightedClasses[i].GetClassWeight();};
    
    RandomChoicer::RandomChoiceWeighted(_weightedClasses, getWeight1, _selectedClasses);
    if (! _selectedClasses.empty())
        return _selectedClasses.back();
    LOG_SEV(warning) << "Класс с подходящими весами не найден. Выбран произвольный класс";
    
    auto getWeight2 = [this](int i)
                        {return _weightedClasses[i].GetClassWeight(0);};
    
    RandomChoicer::RandomChoiceWeighted(_weightedClasses, getWeight2, _selectedClasses);
    if (! _selectedClasses.empty())
        return _selectedClasses.back();
    LOG_SEV(warning) << "Ни один из классов не был выбран по весу";

    RandomChoicer::RandomChoice(_weightedClasses, _selectedClasses);
    if (! _selectedClasses.empty())
        return _selectedClasses.back();
    LOG_SEV(warning) << "Ни один из классов не был выбран";
    
    return BoostingClassInfo(-1);
}

void BoostingLearner::WeightObjects(Dataset const& ds, 
                                             Predictions const& predictions, 
                                             BoostingClassInfo const& cls)
{
    int m = predictions.size();
	//Weight w = 0;
    for (int i = 0; i < m; ++i)
    {
        Weight labelEst = predictions[i].EstimateByLabel(predictions[i].label);
        ds.objects[i]->weight = 0;

        for (auto& p : predictions[i].predictions)
        {
            if (ds.objects[i]->label == cls.label)
            {
                if (p.label != cls.label)
                    ds.objects[i]->weight += exp(p.estimate - labelEst);
            }
            else
            {
                if (p.label != cls.label)
                    continue;                
                ds.objects[i]->weight = exp(p.estimate - labelEst);
                break;
            }
        }        
        
        //ds.objects[i]->weight /= cls.GetTotalWeight();
		//w += ds.objects[i]->weight;
    }

}





Weight BoostingLearner::WeightBinaryClassifier( 
    BoostingClassInfo &cls,
    IBinaryClassifierPtr bincls, 
    ObjectPtrList const& positive, 
    ObjectPtrList const& negative ) const
{
    Weight P = 0;

    for (auto& o : positive)
        P += o->weight * bincls->Apply(*o);

    if (P <= 0)
    {
        LOG_SEV(warning) << "Бинарный классификатор на положительных объектах вернул P=" << P;
        return -1;
    }

    Weight N = 0;
    for (auto& o:negative)
        N += o->weight * bincls->Apply(*o);

    if (N < 0)
    {
        LOG_SEV(warning) << "Бинарный классификатор на отрицательных объектах вернул N=" << N;
        return -1;
    }

    if (P < N)
    {
        LOG_SEV(warning) << "Бинарный классификатор на положительных объектах ошибается чаще, чем на отрицательных (P=" << P << ", N=" << N << ")";
        return -1;
    }

    return cls.GetWeakClassifierWeight(P, N);
}

void BoostingLearner::SetDefaultOptions()
{
    CurrentOptions = BoostingOptions();

    if (! positiveLearner && 
        ! negativeLearner)
    {
        SetupDefaultLearners();
    }
    else
    {
        if (positiveLearner)
            TrySetDefaultOptions(positiveLearner);
        if (negativeLearner)
            TrySetDefaultOptions(negativeLearner);
    }
    
    
}

typedef std::map<std::string, IBinaryClassifierLearnerPtr> LearnersMap;

// Инициализация алгоритмов по конфигурации с указанными именами
LearnersMap CreateLearners(Options const& options, Strings const& names)
{
    LearnersMap learners;

    for (auto& name:names)
    {
        if (name.empty())
            continue;
        
        if (learners[name])
            continue;
        
        learners[name] = TryCreateChild<IBinaryClassifierLearnerPtr>(options, name);
    }

    return std::move(learners);
}

void BoostingLearner::SetOptions( Options const& options )
{
    BoostingOptions def;
    
    CurrentOptions.iterationsCount = options.get("iterationsCount", def.iterationsCount);
    CurrentOptions.stopQ = options.get("stopQ", def.stopQ);
    CurrentOptions.minWeight = options.get("minWeight", def.minWeight);
    CurrentOptions.positiveCount = options.get("positiveCount", def.positiveCount);
    CurrentOptions.positivePart = options.get("positivePart", def.positivePart);
            
    auto pos = options.get("positiveLearner", "");
    auto neg = options.get("negativeLearner", "");

    Strings names;
    if (! pos.empty())
        names.push_back(pos);
    if (! neg.empty())
        names.push_back(neg);
    unique(names);
    
    // Инициализация алгоритмов, на которые есть ссылки
    auto learners = CreateLearners(options, names);
    
    positiveLearner = learners[pos];
    negativeLearner = learners[neg];

    if (! pos.empty() && ! positiveLearner)
    {
        BOOST_THROW_EXCEPTION(error() << message("Классификатор " + pos + " не найден в конфргурации"));
    }
    if (! neg.empty() && ! negativeLearner)
    {
        BOOST_THROW_EXCEPTION(error() << message("Классификатор " + neg + " не найден в конфргурации"));
    }
        

    if (! positiveLearner && ! negativeLearner)
    {
        SetupDefaultLearners();
    }
    
}




BoostingLearner::BoostingLearner()
{
    SetupDefaultLearners();
}

void BoostingLearner::Register()
{
    Registrator<IClassifierLearnerPtr>::Instance().RegisterFactory
        ("Boosting", [](){return std::make_shared<BoostingLearner>();});
}

int BoostingLearner::CalculateIterationCount( Dataset const &ds )
{
    Estimate m = (Estimate)ds.objects.size();
    Estimate l = (Estimate)ds.classes.size();
    Estimate N = 1/m;
    Estimate q = (Estimate)GetCorrectPositiveObjectsCount((int)m);
           
    if (q <= 1)
        q = 2;
        
    if (q > m)
        q = m;
    
    Estimate P = 1 / ceil(m / q);
    Estimate d = 2 * sqr(sqrt(P) - sqrt(N))/l;

/*    Estimate delta = 0;
    Estimate count = 0;

    for (auto& c : ds.classes)
    {
        for (int i = 0; i != 2; ++i)
        {
            if (i == 0 && ! positiveLearner)
                continue;
            
            if (i == 1 && ! negativeLearner)
                continue;

            int mk = i == 0 ? c.positive.size() : c.negative.size();
            Estimate q = (Estimate)GetCorrectPositiveObjectsCount(mk);

            if (q == 0)
                ++q;

            Estimate P = 1/ceil(mk / q);
            
            if (P <= N)
                continue;

            Estimate d = sqr(sqrt(P) - sqrt(N))/l;
            if (delta == 0 ||)
            delta += d;
            count += 1;
        }
    }

    if (count)
        delta /= count;*/

    //LOG_SEV(info) << "delta=" << delta;

    return d > 0 ? (int)ceil(log(m) / d) : (int)m;
}

int BoostingLearner::GetCorrectPositiveObjectsCount( int mk ) const
{
    int q = CurrentOptions.positivePart > 0 && CurrentOptions.positivePart <= 1 ? 
        (int)floor(mk * CurrentOptions.positivePart) :
        CurrentOptions.positiveCount;

    if (q <= 0)
        return 1;
    if (q > mk)
        return mk;
    return q;
}

void BoostingLearner::SetupDefaultLearners()
{
    positiveLearner = std::make_shared<ECVotingLearner>();
    negativeLearner.reset();
}

void BoostingLearner::CollectCorrectObjects( 
    ObjectsClass& clsref, 
    BoostingClassInfo& clsinfo )
{
    clsref.SortObjectsByWeights();
    
    int mk = clsref.positive.size();
    int q = GetCorrectPositiveObjectsCount(mk);

    Weight P = 0, N = 0;
    int posn = 0;
    for (;posn < q || (! clsinfo.informativity.Sucess(P, N) && posn != mk); ++posn)
    {
        P += clsref.positive[posn]->weight;
    }

    int negn = clsref.negative.size();
    int negn0 = negn;
    for (; negn > posn; --negn)
    {
        N += clsref.negative[negn-1]->weight;
        if (! clsinfo.informativity.Sucess(P, N))
            break;
    }

    _correctPart.label = clsref.label;
    _correctPart.positive = ObjectsRange(clsref.positive, 0, posn);
    _correctPart.negative = ObjectsRange(clsref.negative, 0, negn);

    _fittingPart.label = clsref.label;
    _fittingPart.positive = ObjectsRange(clsref.positive, posn, mk);
    _fittingPart.negative = ObjectsRange(clsref.negative, negn, negn0);
        
}

void BoostingClassInfo::AddNegativeWeight( Weight w )
{
    negativeWeight += w;
    if (informativity.minN == 0 || informativity.minN > w)
        informativity.minN = w;
}

void BoostingClassInfo::AddPositiveWeight( Weight w )
{
    positiveWeight += w;
    if (informativity.minP == 0 || informativity.minP > w)
        informativity.minP = w;
}

Weight BoostingClassInfo::GetWeakClassifierWeight( Weight p, Weight n )
{
    if (n < informativity.minN)
        n = informativity.minN;

    if (p <= n || p <= 0 || n <= 0)
        return 0;

    return (log(p) - log(n)) / 2;
}

Weight BoostingClassInfo::NormalizeWeight( Weight w ) const
{
    return GetTotalWeight() != 0 ? w / GetTotalWeight() : 0;
}

BoostingClassInfo::BoostingClassInfo( int l /*= -1*/, bool c /*= false*/ )
{
    label = l;
    complement = c;
    positiveWeight = 0;
    negativeWeight = 0;
    informativity.type = InformativityCondition::AdaBoost;
    Q = 0;
}

BoostingClassInfo BoostingClassInfo::GetComplement() const
{
    BoostingClassInfo c = *this;
    c.complement = ! c.complement;
    std::swap(c.positiveWeight, c.negativeWeight);
    std::swap(c.informativity.minN, c.informativity.minP);
    return c;
}

Weight BoostingClassInfo::GetClassWeight( Weight threshold ) const
{
    auto inf = informativity.Get(positiveWeight, 0);
    if (inf > threshold)
        return inf;
    return 0;
}

Weight BoostingClassInfo::GetClassWeight() const
{
    return GetClassWeight(informativity.threshold);
}

Weight BoostingClassInfo::GetTotalWeight() const
{
    return positiveWeight + negativeWeight;
}

Weight BoostingClassInfo::GetNormalizedPositiveWeight() const
{
    return NormalizeWeight(positiveWeight);
}

Weight BoostingClassInfo::GetNormalizedNegativeWeight() const
{
    return NormalizeWeight(negativeWeight);
}