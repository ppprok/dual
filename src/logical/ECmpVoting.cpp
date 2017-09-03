#include "stdafx.h"

#include "ECmpVoting.h"


Estimate ECmpVoting::Apply( Object const& o ) const
{
    Estimate e = 0, me = 0;
    for (auto& p : ecmpSets)
    {
        e += p.Apply(precedents, o);
        me += precedents.size() - p.excludedPrecedents.size();
    }

    if (e && me)
        e /= me;
    return e;
}

ECmpVoting::ECmpVoting( ECmpSetInfos ecset_, ObjectPtrList precedents_ )
{
    ecmpSets = std::move(ecset_);
    precedents = std::move(precedents_);
}

void ECmpVoting::GetECSet( ECSet& ecset )
{
    for (auto& ecmpSet:ecmpSets)
        for (auto& ecmp:ecmpSet.ecmpSet)
            ecset.emplace_back(ecmp.ec);
}


Estimate ECmpSetInfo::Apply( Object const& precedent, Object const& o ) const
{
    for (auto& ecmp:ecmpSet)
    {
        if (! ecmp.Apply(precedent, o))
            return 0;
    }
    return 1;
}

Estimate ECmpSetInfo::Apply( ObjectPtrList const& precedents, Object const& o ) const
{
    Estimate e = 0;

    for (auto& p : precedents)
    {
        if (contains(excludedPrecedents, p))
            continue;
        e += Apply(*p, o);
    }

    return e;
}

ECmpVoting0::ECmpVoting0( ECmpSets ecmpSets_, ObjectPtrList precedents_)
    :ecmpSets(std::move(ecmpSets_)), precedents(std::move(precedents_))
{
}

Estimate ECmpVoting0::Apply( Object const& o ) const
{
    Estimate e = 0, me = 0;
    for (auto const& ecmpSet : ecmpSets)
    {
        for (auto const& p : precedents)
        {
            e += Apply(ecmpSet, *p, o);
            ++me;
        }        
    }

    if (e && me)
        e /= me;
    
    return e;
}


Estimate ECmpVoting0::Apply( ECmpSet const& ecmpSet, Object const& p, Object const& o ) const
{
    for (auto& ecmp :  ecmpSet)
    {
        if (! ecmp.Apply(p,o))
            return 0.0f;
    }
    return 1.0f;
}

void ECmpVoting0::GetECSet( ECSet& ecset )
{
    for (auto& ecmpSet:ecmpSets)
        for (auto& ecmp:ecmpSet)
            ecset.emplace_back(ecmp.ec);
}