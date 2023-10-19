#include "PSVotingLearner.h"
#include "ECVoting.h"


IBinaryClassifierPtr PSVotingLearner::Train(
    ObjectsClassRange const& correct, 
    ObjectsClassRange const& fitting)
{
    _featureSelector->Select(correct, fitting);
    InitFeatureComparer();
    
    if (correct.positive.size() > 1)
    {
        auto cc = std::make_shared<ComparersComposer>();
        cc->comparers.emplace_back(_featureComparer);
        
        auto oo = std::make_shared<OptionalObjects>();
        oo->optionalPrecedents.assign(correct.positive.begin(), correct.positive.end());
        cc->comparers.emplace_back(oo);

        _cmpFitMatrixBuilder.SetComparer(cc);
        _cmpFitMatrixBuilder.Build(correct, fitting);
        _cmpFitMatrixBuilder.FindCovers();

        _ecset.clear();    
        CollectECSet(correct.positive, *oo, *cc);
    }
    else
    {
        _cmpFitMatrixBuilder.SetComparer(_featureComparer);
        _cmpFitMatrixBuilder.Build(correct, fitting);
        _cmpFitMatrixBuilder.FindCovers();

        _ecset.clear();    
        CollectECSet(correct.positive);
    }
    
    MergeCollectedECSet();    
    return std::make_shared<ECVoting>(_ecset);
}

void PSVotingLearner::Register()
{
    Registrator<IBinaryClassifierLearnerPtr>::Instance().RegisterFactory
        ("PSVoting", [](){return std::make_shared<PSVotingLearner>();});

}

void PSVotingLearner::MergeCollectedECSet()
{
    sort(_ecset);

    int m = _ecset.size();
    if (m > 1)
    {
        int z = 0;
        for (int i = 1; i != m; ++i)
        {
            if (! equal(_ecset[i].featureValues, _ecset[i-1].featureValues))
                continue;

            _ecset[i].weight += _ecset[i-1].weight;
            _ecset[i-1].weight = 0;
            ++z;
        }

        if (z)
            remove_if(_ecset, [](EC const& ec){return ec.weight == 0;});
    }
}

void PSVotingLearner::CollectECSet(ObjectsRange const& objects)
{

    for (auto cover : _cmpFitMatrixBuilder.GetCoverMatrix().GetCovers().get_rows())
    {
        _features.clear();
        _featureComparer->Get(cover, _features);

        for (auto po : objects)
            _ecset.emplace_back(_features, po->values, 1.0f);
    }

}

void PSVotingLearner::CollectECSet( ObjectsRange const& objects, 
                                   OptionalObjects& oo, ComparersComposer& cc )
{
    _excluded.clear();
    _excluded.reserve(objects.size());

    for (auto cover : _cmpFitMatrixBuilder.GetCoverMatrix().GetCovers().get_rows())
    {
        _excluded.clear();
        oo.Get(cc.GetPart(cover, 1), _excluded);
        if (_excluded.size() == objects.size())
            continue;
        _features.clear();
        _featureComparer->Get(cc.GetPart(cover, 0), _features);

        for (auto po : objects)
        {
            if (contains(_excluded, po))
                continue;
            
            _ecset.emplace_back(_features, po->values, 1.0f);
        }
    }

}