#include "stdafx.h"

#include "ECmpSetPredicateBuilder.h"




void ECmpSetPredicateBuilder::Train( ECmpSetPredicateBuilder::TrainInfo const& positive, 
                                     ECmpSetPredicateBuilder::TrainInfo const& negative, int count )
{
    

    CmpMatrixSubmatrix weightedSubmatrix = 
        _cmpMatrixBuilder.GetSubmatrix(
        positive.correctObjects, join(positive.weightedObjects, negative.weightedObjects),
        positive.optionalObjects, negative.optionalObjects);

    CmpMatrixSubmatrix correctSubmatrix = _cmpMatrixBuilder.GetSubmatrix(
        positive.correctObjects, negative.correctObjects,
        positive.optionalObjects, negative.optionalObjects);

    Weight maxN = 0, maxP = 0;

    for(auto& o:negative.weightedObjects)
    {
        maxN += o->weight;
    }
    maxN *= correctSubmatrix.Positive.size();

    for(auto& o:positive.weightedObjects)
    {
        maxP += o->weight;
        o->weight = -o->weight;
    }
    maxP *= correctSubmatrix.Positive.size();

    int m = weightedSubmatrix.Rows.size();
    Weights weights;
    weights.reserve(m);
    for (int i = 0; i < m; ++i)
    {
        weights.push_back(
            weightedSubmatrix.Negative[weightedSubmatrix.GetNegNum(i)]->weight);
    }        
    for(auto& o:positive.weightedObjects)
        o->weight = -o->weight;

    CoveredRowWeighterCallback weighter;
    weighter.SetTargetRows(std::move(weightedSubmatrix.Rows));
    weighter.SetWeights(std::move(weights));

    MostCoverCollectorCallback collector;
    collector.Weighter = &weighter;
    collector.Options.TrimOrder = 1;
    collector.Options.MostCount = count;
    collector.Options.TrimSize = count*2;

    Runc::GlobalState A(Runc::RUNCM);
    A.SetMatrix(bit_matrix(_cmpMatrixBuilder.GetCmpMatrix().width(), correctSubmatrix.Rows));
    A.InnerCallback = &weighter;
    A.CoverCallback = &collector;
    A.Dualize();
    collector.Trim();

    ObjectPtrList objset1;
    ObjectPtrList objset2;

    for (auto& cover: collector.GetCovers())
    {   
        _cmpMatrixBuilder.GetExcludedOptionalObjects(
            correctSubmatrix, cover.columns, 
            objset1, objset2);

        objset2 = correctSubmatrix.Positive;
        exclude(objset2, objset1);

        if (objset2.empty())
            continue;

        _predicates.emplace_back(objset2, 
            _cmpMatrixBuilder.GetECmpSubset(cover.columns), 
            maxP - maxN + cover.weight);
    }
}

void ECmpSetPredicateBuilder::Init( ObjectPtrList const& positive, ObjectPtrList const& negative, ECmpSet localBasis, int maxOptional /*= 0*/ )
{
    _cmpMatrixBuilder.Build(positive, negative, localBasis, maxOptional);
}

ECmpSetPredicates const& ECmpSetPredicateBuilder::GetPredicates() const
{
    return _predicates;
}

ECmpSetPredicates& ECmpSetPredicateBuilder::GetPredicates()
{
    return _predicates;
}