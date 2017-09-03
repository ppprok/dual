#include "stdafx.h"

#include "ECVotingLearner.h"
#include "Registrator.h"
//#include "Exceptions.h"
#include "ECVoting.h"



IBinaryClassifierPtr ECVotingLearner::Train( 
    ObjectsClassRange const& correct, 
    ObjectsClassRange const& fitting)
{
    _featureSelector->Select(correct, fitting);
    InitFeatureComparer();
    
    _ecset.clear();    

    int i = 0;
    for (auto& po : correct.positive)
    {
        if (AnyExtract(*po))
        {
            ++i;
            continue;
        }
        
        ObjectsClassRange correct1;
        
        correct1.negative = correct.negative;    
        correct1.positive = ObjectsRange(correct.positive, i, i+1);

        _cmpFitMatrixBuilder.SetComparer(_featureComparer);
        _cmpFitMatrixBuilder.Build(correct1, fitting);

        ObjectsClass empty;
        ObjectsClassRange fitting1(empty);
        fitting1.positive = ObjectsRange(correct.positive, 0, i);
        if (! fitting1.positive.empty())
            _cmpFitMatrixBuilder.Build(correct1, fitting1);

        fitting1.positive = ObjectsRange(correct.positive, i+1, correct.positive.size());
        if (! fitting1.positive.empty())
            _cmpFitMatrixBuilder.Build(correct1, fitting1);
        
        ++i;

        _cmpFitMatrixBuilder.FindCovers();

        CollectECSet(correct1.positive);
    }

    //MergeCollectedECSet();
    
    for (auto& ec:_ecset)
    {
        ec.weight = 1;
    }

    WeightECSet(_ecset, correct.positive);
    WeightECSet(_ecset, fitting.positive);
        
    LOG_SEV(trace) << "ecset.size()=" << _ecset.size();
    return std::make_shared<ECVoting>(_ecset);
}


void ECVotingLearner::Register()
{
    Registrator<IBinaryClassifierLearnerPtr>::Instance().RegisterFactory
        ("ECVoting", [](){return std::make_shared<ECVotingLearner>();});
}

void ECVotingLearner::WeightECSet(ECSet& ecset, ObjectsRange const& objects )
{
    for (auto& o:objects)
    {
        auto w = o->weight;
        if (! w)
            w = 1;
        
        for (auto& ec: ecset)
        {
            if (ec.Apply(*o))
            {
                ec.weight += w;
            }
        }
    }
}

bool ECVotingLearner::AnyExtract( Object const& o ) const
{
    for (auto& ec:_ecset)
    {
        if (ec.Apply(o))
        {
            return true;
        }
    }

    return false;
}
