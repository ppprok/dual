#include "stdafx.h"

#include "BoostingLogicalCorrector.h"
#include "math_utils.h"


Prediction BoostingLogicalCorrector::Apply(Object const& o) const
{
    Prediction r(o.label);

    for (auto& cls : _predicates)
    {
        ClassPrediction cp(cls.first);

        for (auto& p: cls.second)
        {
            cp.estimate += p.Apply(o, _currentOptions.mode == Additive);
        }

        r.predictions.push_back(std::move(cp));
    }

    return std::move(r);
}

void BoostingLogicalCorrector::InitClassesPredicates(Dataset const &ds)
{
    for (auto& c: ds.classes)
    {
        _predicates[c.label].reserve(_currentOptions.iterationsCount);
    }
}
/*
void BoostingLogicalCorrector::CalculateWeightsByMargins(Dataset const &ds, Predictions &predictions)
{
    assert(ds.objects.size() == predictions.size());

    int m = ds.objects.size();
    for (int i = 0; i < m; ++i)
    {
        predictions[i].CalculateMargins();
        Estimate margin = predictions[i].predictions[ds.objects[i]->label].margin;
        ds.objects[i]->weight = exp(-margin);
    }
}*/
/*
void BoostingLogicalCorrector::SelectOptionalPositiveObjects(SamplesWeighter &weighter, SelectedClassInfo cls, int count, ECmpSetPredicateBuilder::TrainInfo &positive)
{
    positive.optionalObjects = std::move(weighter.GetRandomSamples(count * 2, cls));
    unique(positive.optionalObjects, false);
    exclude(positive.optionalObjects, positive.correctObjects);
    if ((int)positive.optionalObjects.size() > count)
        positive.optionalObjects.resize(count);

    positive.correctObjects.insert(positive.correctObjects.end(),
        positive.optionalObjects.begin(),
        positive.optionalObjects.end());
}*/

/*void BoostingLogicalCorrector::AssignWeightedObjects(Dataset const &ds, SelectedClassInfo &cls, ECmpSetPredicateBuilder::TrainInfo &positive, ECmpSetPredicateBuilder::TrainInfo &negative)
{
    Class const& clsObjects = ds.GetClass(cls.label);
    if (cls.negative)
    {
        positive.weightedObjects = clsObjects.negative;
        negative.weightedObjects = clsObjects.positive;
    }
    else
    {
        positive.weightedObjects = clsObjects.positive;
        negative.weightedObjects = clsObjects.negative;
    }
}*/

void BoostingLogicalCorrector::Train(Dataset const& ds)
{
    _currentOptions = CreateCurrentOptions(ds);
    
    InitClassesPredicates(ds);

    auto predictions = ApplyMany(ds.objects);
        
    int m = ds.objects.size();    
    Weight delta = log(m) / _currentOptions.iterationsCount;
    
    ECmpSetPredicateBuilder predicateBuilder;
    
    predicateBuilder.Init(ds.objects, ds.objects, 
        _currentOptions.localBasis, 
        _currentOptions.optionalObjectsCount);

    ECmpSetPredicateBuilder::TrainInfo positive;
    ECmpSetPredicateBuilder::TrainInfo negative;

	bool add = _currentOptions.mode == Mode::Additive;

    for (int t = 0; t < _currentOptions.iterationsCount; ++t)
    {
        auto cls = SelectClass(ds, predictions, delta, _currentOptions.epsilon);
		if (cls.Q <= _currentOptions.stopQ)
			break;

        WeightObjects(ds, predictions, cls);

        SelectTrainObjects(ds, cls, positive, negative, delta, _currentOptions.epsilon, _currentOptions.optionalObjectsCount);

		predicateBuilder.GetPredicates().clear();
        predicateBuilder.Train(positive, negative, 10);
        if (predicateBuilder.GetPredicates().empty())
            break;

        auto predicate = std::move(predicateBuilder.GetPredicates().front());
        predicate.weight = 1.0;//add ? 1.0 / predicate.precedents.size() : 1.0;

		auto& c = ds.GetClass(cls.label);

        Weight P = 
			predicate.ApplyMany(c.positive, add);
        Weight N = 
			predicate.ApplyMany(c.negative, add);

		if (cls.complement)
			std::swap(P,N);
				
        predicate.weight = 0.5 * (log(P) - log(N + _currentOptions.epsilon));

		if (predicate.weight <=0 )
			continue;

        if (cls.complement)
            predicate.weight = -predicate.weight;
        
		int k = predictions.front().PosByLabel(cls.label);

        for (int i = 0; i < m; ++i)
        {
            Estimate e = predicate.Apply(*ds.objects[i], add);
			if (e)
				predictions[i].predictions[k].estimate += e;
        }

        _predicates[cls.label].push_back(std::move(predicate));
    }
}

void BoostingLogicalCorrector::Reset()
{
    _predicates.clear();
}

BoostingLogicalCorrector::Options BoostingLogicalCorrector::CreateCurrentOptions(Dataset const& ds)
{
    Options o = RecommendedOptions;
    
    if (o.epsilon <= 0 || o.epsilon >= 1/2)
        o.epsilon = 0.5 / ds.objects.size();

    if (o.iterationsCount <= 0)
        o.iterationsCount = (int)ceil(2 * ds.classes.size() * log(ds.objects.size()) / ClassWeightInfo::psi2(0.5, 0, o.epsilon));

    if (o.optionalObjectsCount < 0)
        o.optionalObjectsCount = 0;

    if (o.localBasis.empty())
    {
        LocalBasisBuilder lbb;
        lbb.AddOneRankECmpSet(ds.featureValues, ECmp::LE);
        lbb.AddOneRankECmpSet(ds.featureValues, ECmp::GE);
        //lbb.AddOneRankECmpSet(ds.featureValues, ECmp::Or);
        //lbb.AddOneRankECmpSet(ds.featureValues, ECmp::NotAnd);

        o.localBasis = std::move(lbb.ecset);
    }

    return std::move(o);
}



std::vector<BoostingLogicalCorrector::ClassWeightInfo> 
    BoostingLogicalCorrector::CollectClassWeightInfo(
    Classes const& classes, Predictions const& predictions, Weight eps)
{
    std::vector<ClassWeightInfo> infos;
    infos.reserve(classes.size()*2);

    for(auto& c:classes)
    {
        infos.emplace_back(c.label);
    }

    int l = classes.size();
    Weight Q = 0;
	Weight Q2 = 0;

    for (auto& o : predictions)
    {
        int labelPos = o.PosByLabel(o.label);
		assert(inside(infos, labelPos) && infos[labelPos].label == o.label);

		Weight labelEst = o.predictions[labelPos].estimate;
		
        for (int i = 0; i < l; ++i)
        {
			assert(o.predictions[i].label == infos[i].label);
			
			if (infos[i].label == o.label)
				continue;			
                
			Weight w = exp(- labelEst + o.predictions[i].estimate);

			infos[i].negativeWeight += w;
			infos[labelPos].positiveWeight += w;

			Q += w;            
        }        
    }


    for (int i = 0; i < l; ++i)
    {
        /*infos[i].totalWeight = infos[i].positiveWeight + infos[i].negativeWeight;
        if (infos[i].totalWeight > 0)
        {
            infos[i].positiveWeight /= infos[i].totalWeight;
            infos[i].negativeWeight /= infos[i].totalWeight;
        }*/
		infos[i].Q = Q;

		Q2 += infos[i].GetTotalWeight();
        
        infos.push_back(infos[i]);
        infos.back().complement = true;
        std::swap(infos.back().positiveWeight, infos.back().negativeWeight);
    }
	   
	std::cout << Q << std::endl;

    return std::move(infos);
}

BoostingLogicalCorrector::ClassWeightInfo 
    BoostingLogicalCorrector::SelectClass(
    Dataset const &ds, 
    Predictions const& predictions, 
    Weight delta,
	Weight eps)
{
    auto weightedClasses = CollectClassWeightInfo(ds.classes, predictions, eps);
    
    std::vector<ClassWeightInfo> selected;

    auto getWeight1 = [&weightedClasses, delta, eps](int i)
                        {return weightedClasses[i].GetMaxDelta(eps) > delta 
                                ? weightedClasses[i].positiveWeight : 0;};
    
    _randomizer.RandomChoiceWeighted(weightedClasses, getWeight1, selected);
    if (! selected.empty())
        return selected.back();
    
    auto getWeight2 = [&weightedClasses](int i)
                        {return weightedClasses[i].positiveWeight;};            
    _randomizer.RandomChoiceWeighted(weightedClasses, getWeight2, selected);
    if (! selected.empty())
        return selected.back();

    return ClassWeightInfo(-1);
}

void BoostingLogicalCorrector::WeightObjects(Dataset const& ds, 
                                             Predictions const& predictions, 
                                             BoostingLogicalCorrector::ClassWeightInfo const& cls)
{
    int m = predictions.size();
	Weight w = 0;
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
        
        ds.objects[i]->weight /= cls.GetTotalWeight();
		w += ds.objects[i]->weight;
    }


}

void BoostingLogicalCorrector::SelectTrainObjects(
    Dataset const& ds,
    BoostingLogicalCorrector::ClassWeightInfo const& cls, 
    ECmpSetPredicateBuilder::TrainInfo& positive, 
    ECmpSetPredicateBuilder::TrainInfo& negative, 
    Weight delta, Weight epsilon, int optionalObjectsCount)
{
    positive.Clear();
    negative.Clear();

	auto& c = ds.GetClass(cls.label);
	assert(c.label == cls.label);
    
	if (! cls.complement)
	{
		positive.correctObjects = c.positive;
		negative.correctObjects = c.negative;
	}
	else
	{
		positive.correctObjects = c.negative;
		negative.correctObjects = c.positive;
	}


    _randomizer.RandomShuffle(positive.correctObjects);
    _randomizer.RandomShuffle(negative.correctObjects);

    ds.OrderByWeightDescent(positive.correctObjects);
    ds.OrderByWeightDescent(negative.correctObjects);

    Weight posw = cls.GetNormalizedPositiveWeight();
    Weight negw = 0;

    for(;;)
    {
        Weight posDelta = positive.correctObjects.size() <= 1 ? 0 : 
            cls.GetDelta(posw - positive.correctObjects.back()->weight, negw, epsilon);
        
        Weight negDelta = negative.correctObjects.size() <= 1 ? 0 : 
            cls.GetDelta(posw, negw + negative.correctObjects.back()->weight, epsilon);

        bool removePositive = false;
        bool removeNegative = false;

        if (posDelta > delta && negDelta > delta)
        {
            if (negative.correctObjects.size() > positive.correctObjects.size() ||
                (negative.correctObjects.size() == positive.correctObjects.size() &&
                 negative.correctObjects.back()->weight > positive.correctObjects.back()->weight))
            {
                removePositive = true;
            }
            else
            {
                removeNegative = true;
            }
        }
        else
        {
            removePositive = posDelta > delta;
            removeNegative = negDelta > delta;            
        }

        if (removePositive)
        {
            posw -= positive.correctObjects.back()->weight;
            positive.weightedObjects.push_back(positive.correctObjects.back());
            positive.correctObjects.pop_back();                
        }
        else if (removeNegative)
        {
            negw += negative.correctObjects.back()->weight;
            negative.weightedObjects.push_back(negative.correctObjects.back());
            negative.correctObjects.pop_back();
        }
        else
        {
            break;
        }
    }

    CollectOptionalObjects(optionalObjectsCount, positive);
}



void BoostingLogicalCorrector::SelectTrainObjectsByIncrease(
	Dataset const& ds,
	BoostingLogicalCorrector::ClassWeightInfo const& cls, 
	ECmpSetPredicateBuilder::TrainInfo& positive, 
	ECmpSetPredicateBuilder::TrainInfo& negative, 
	Weight delta, Weight epsilon, int optionalObjectsCount)
{
	positive.Clear();
	negative.Clear();

	auto& c = ds.GetClass(cls.label);
	assert(c.label == cls.label);

	if (! cls.complement)
	{
		positive.weightedObjects = c.positive;
		negative.weightedObjects = c.negative;
	}
	else
	{
		positive.weightedObjects = c.negative;
		negative.weightedObjects = c.positive;
	}

	if (positive.weightedObjects.empty())
		return;

	if (negative.weightedObjects.empty())
		return;

	_randomizer.RandomShuffle(positive.weightedObjects);
	_randomizer.RandomShuffle(negative.weightedObjects);

	ds.OrderByWeight(positive.weightedObjects);
	ds.OrderByWeight(negative.weightedObjects);

	positive.correctObjects.push_back(positive.weightedObjects.back());
	negative.correctObjects.push_back(negative.weightedObjects.back());
	positive.weightedObjects.pop_back();
	negative.weightedObjects.pop_back();
	
	Weight posw = positive.correctObjects.front()->weight;
	Weight negw = cls.GetNormalizedNegativeWeight() - negative.correctObjects.front()->weight;

	Weight diff = cls.GetDeltaDiff(posw, negw, epsilon, delta);

	while(diff <= 0 && (!positive.weightedObjects.empty() || ! negative.weightedObjects.empty()))
	{
		Weight posDelta = positive.weightedObjects.empty() ? diff : cls.GetDeltaDiff(posw + positive.weightedObjects.back()->weight, negw, epsilon, delta);
		Weight negDelta = negative.weightedObjects.empty() ? diff : cls.GetDeltaDiff(posw, negw - negative.weightedObjects.back()->weight, epsilon, delta);
	
		if (posDelta > negDelta || (posDelta == negDelta &&
			positive.correctObjects.size() > negative.correctObjects.size()))
		{
			posw += positive.weightedObjects.back()->weight;
			positive.correctObjects.push_back(positive.weightedObjects.back());
			positive.weightedObjects.pop_back(); 
			diff = posDelta;
		}
		else
		{
			negw -= negative.weightedObjects.back()->weight;
			negative.correctObjects.push_back(negative.weightedObjects.back());
			negative.weightedObjects.pop_back(); 
			diff = negDelta;
		}
		
		
	}

	CollectOptionalObjects(optionalObjectsCount, positive);

}

int BoostingLogicalCorrector::CollectOptionalObjects(int optionalObjectsCount, ECmpSetPredicateBuilder::TrainInfo &positive)
{
	if (optionalObjectsCount > 0)
	{
		if ((int)positive.weightedObjects.size() < optionalObjectsCount)
			optionalObjectsCount = positive.weightedObjects.size();

		positive.optionalObjects.assign(
			positive.weightedObjects.end() - optionalObjectsCount, 
			positive.weightedObjects.end());

		positive.correctObjects.insert(positive.correctObjects.end(), positive.optionalObjects.begin(), positive.optionalObjects.end());
	}	
	return optionalObjectsCount;
}

