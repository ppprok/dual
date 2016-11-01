#include "stdafx.h"

#include "FeatureSelectorBase.h"


bool FeatureSelectorBase::IsDefectPair(IFeatureSelector const& fs, Object const& po, Object const& no )
{
    
    for (auto& ecmp: fs.GetECmpSet())
        if (! ecmp.Apply(po, no))
            return false;

    for(auto j: fs.GetFeatures())
    {
        if (po.HasFeature(j) && no.HasFeature(j) &&
            po.values[j] != no.values[j])
            return false;
    }

    return true;
}

int FeatureSelectorBase::CountDefectPairs(IFeatureSelector const& fs, ObjectsRange const& positive, ObjectsRange const& negative, int maxc /*= -1*/ )
{
    
    int c = 0;

    for (auto& po : positive)
        for (auto& no : negative)
            if (IsDefectPair(fs,*po, *no))
            {
                ++c;
                if (maxc != -1)
                {
                    --maxc;
                    if (maxc == -1)
                        return c;
                }                    
            }

     return c;
}

bool FeatureSelectorBase::AnyDefectPairs(IFeatureSelector const& fs, ObjectsRange const& positive, ObjectsRange const& negative )
{
    
    for (auto& po:positive)
        for (auto& no : negative)
            if (IsDefectPair(fs,*po, *no))
                return true;
    return false;
}


ECmpSet const& FeatureSelectorBase::GetECmpSet() const
{
    return _ecmpSet;
}

Features const& FeatureSelectorBase::GetFeatures() const
{
    return _features;
}



void FeatureSelectorBase::Reset()
{

}

void FeatureSelectorBase::Init( Dataset const& ds )
{
    
}

Weight FeatureSelectorBase::WeightDefectPairs( IFeatureSelector const& fs, ObjectsRange const& positive, ObjectsRange const& negative )
{
    Weight defect = 0;
    Weight total = 0;


    for (auto& po:positive)
    {
        if (! po->weight)
            continue;
        
        for (auto& no : negative)
        {
            if (! no->weight)
                continue;
            
            Weight w = po->weight * no->weight;
            if (IsDefectPair(fs,*po, *no))
            {
                defect += w;
            }

            total += w;
        }
    }
    
    if (defect && total)
        defect /= total;
    
    return defect;
}