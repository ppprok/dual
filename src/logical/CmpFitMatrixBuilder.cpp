
#include "CmpFitMatrixBuilder.h"
#include "RUNC.h"
#include "Interfaces.h"



void CmpFitMatrixBuilder::MergeCmpMatrixRows(int thres, int limit)
{
    if (_cmpMatrix.GetCmpMatrix().height() <= thres)
        return;

    int m = _cmpMatrix.GetCmpMatrix().height();

    while (m > limit)
    {
        _cmpMatrix.MergeSimilarRows(limit, _fitMatrix);

        if (m == _cmpMatrix.GetCmpMatrix().height())
            break;

        m = _cmpMatrix.GetCmpMatrix().height();
    }
}

void CmpFitMatrixBuilder::MergeFitMatrixRows(int thres, int limit)
{
    if (_fitMatrix.GetFitMatrix().height() <= thres)
        return;
    _fitMatrix.RemoveZeroWeightRows();
    _fitMatrix.RemoveSupRows(_cmpMatrix.GetCmpMatrix().get_rows());

    while (_fitMatrix.GetFitMatrix().height() > limit)
    {
        int m0 = _fitMatrix.GetFitMatrix().height();
        _fitMatrix.MergeSimilarRows(limit);
        if (m0 == _fitMatrix.GetFitMatrix().height())
            break;        
    }
}

void CmpFitMatrixBuilder::Build(ObjectsClassRange const& correct, ObjectsClassRange const& fitting)
{
    for (auto& po : correct.positive)
    {
        _fitMatrix.AddFittingObjects(*po, ObjectsRange(fitting.negative, 0, Options.fitObjectLimit), 1);
        _fitMatrix.AddFittingObjects(*po, ObjectsRange(fitting.positive, 0, Options.fitObjectLimit), -1);        
        _fitMatrix.AddFittingObjects(*po, ObjectsRange(correct.positive, 0, Options.fitObjectLimit), -1);

        MergeFitMatrixRows(Options.fitHeightThreshold, Options.fitHeightLimit);
        
        _cmpMatrix.AddComparableObjects(*po, correct.negative);
        MergeCmpMatrixRows(Options.cmpHeightThreshold, Options.cmpHeightLimit);
    }

    MergeFitMatrixRows(Options.fitHeightLimit, Options.fitHeightLimit);
    MergeCmpMatrixRows(Options.cmpHeightLimit, Options.cmpHeightLimit);
}

void CmpFitMatrixBuilder::SetComparer(IComparerPtr comparer)
{
    _cmpMatrix.SetComparer(comparer);
    _fitMatrix.SetComparer(comparer);
    _coverMatrix.SetWidth(comparer->GetSize());
}


struct DualCallback
    :IDualizationCallback
{
    int iterationCount;

    CmpFitMatrixBuilder* owner;

    bit_vector bits;

    DualCallback(CmpFitMatrixBuilder* o, int n)
        :owner(o), iterationCount(0), bits(n)
    {
    }

    virtual void Call(DualizationNode& node)
    {
        if (! node.IsCover())
            return;
        
        bits.clear();
        bits.set_bits(node.columns);

        Weight w = 0; 
        owner->_coverMatrix.AddCover(bits, owner->GetCoverWeight(bits));

        owner->RestMostCovers();

        ++iterationCount;
        if (iterationCount > owner->Options.iterationLimit)
            node.stoped = true;
    }

};

void CmpFitMatrixBuilder::FindCovers()
{
    DualCallback callback(this, _cmpMatrix.GetCmpMatrix().width());

    Runc::GlobalState dualizer;
    dualizer.CoverCallback = &callback;
    dualizer.SetMatrix(std::move(_cmpMatrix.GetCmpMatrix()));
    dualizer.Dualize();

    RestMostCovers(Options.coverLimit, Options.coverLimit);
}

void CmpFitMatrixBuilder::RestMostCovers(int thres, int limit)
{
    if (_coverMatrix.GetCovers().height() < thres)
        return;

    _coverMatrix.RestMost(limit);
}

void CmpFitMatrixBuilder::RestMostCovers()
{
    RestMostCovers(Options.coverThreshold, Options.coverLimit);
}

void CmpFitMatrixBuilder::SetOptions(::Options const& options )
{
    CmpFitMatrixBuilderOptions def;

    Options.cmpHeightLimit      = options.get("cmpHeightLimit",     def.cmpHeightLimit);
    Options.cmpHeightThreshold  = options.get("cmpHeightThreshold", def.cmpHeightThreshold);
    Options.coverLimit          = options.get("coverLimit",         def.coverLimit);
    Options.coverThreshold      = options.get("coverThreshold",     def.coverThreshold);
    Options.fitHeightLimit      = options.get("fitHeightLimit",     def.fitHeightLimit);
    Options.fitHeightThreshold  = options.get("fitHeightThreshold", def.fitHeightThreshold);
    Options.iterationLimit      = options.get("iterationLimit",     def.iterationLimit);

}

void CmpFitMatrixBuilder::SetDefaultOptions()
{
    Options = CmpFitMatrixBuilderOptions();
}

Weight CmpFitMatrixBuilder::GetCoverWeight( bit_chunk const& cover ) const
{
    if (_fitMatrix.GetFitMatrix().height() > 0)
        return _fitMatrix.GetCoveredRowsWeight(cover);

    return (Weight)-cover.count();
}