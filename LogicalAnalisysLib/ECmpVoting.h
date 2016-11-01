#pragma once

#include "Interfaces.h"
#include "ECmp.h"

struct ECmpSetInfo;

typedef std::vector<ECmpSetInfo> ECmpSetInfos;

// Голосование по набором эл.кл. с отношениями
class ECmpVoting : public IBinaryClassifier, public IECVoting
{
public:

    ECmpSetInfos ecmpSets;
    
    ObjectPtrList precedents;

    ECmpVoting(ECmpSetInfos ecset, ObjectPtrList precedents);

    // Провести голосование по наборам эл.кл.
    virtual Estimate Apply( Object const& o ) const;

    virtual void GetECSet( ECSet& ecset );
};

typedef std::vector<ECmpSet> ECmpSets;

// Голосование по набором эл.кл. с отношениями
class ECmpVoting0 : public IBinaryClassifier, public IECVoting
{
public:

    ECmpSets ecmpSets;

    ObjectPtrList precedents;

    ECmpVoting0(ECmpSets ecsets, ObjectPtrList precedents);

    // Провести голосование по наборам эл.кл.
    virtual Estimate Apply( Object const& o ) const;

    Estimate Apply(ECmpSet const& ecmpSet, Object const& p, Object const& o) const;

    virtual void GetECSet( ECSet& ecset );
};


// Информация о наборе эл.кл. и прецедентах, с которыми не надо осуществлять сравнение
struct ECmpSetInfo
{
    // Набор эл.кл. с отношениями
    ECmpSet ecmpSet;

    // Набор объектов, с которыми не надо производить сравнение
    ObjectPtrList excludedPrecedents;

    ECmpSetInfo(){};

    ECmpSetInfo(ECmpSet e, ObjectPtrList p)
    {
        ecmpSet = std::move(e);
        excludedPrecedents = std::move(p);
    };

    UTILITY_MOVE_DEFAULT_MEMBERS(ECmpSetInfo, (ecmpSet)(excludedPrecedents));

    // Сравнить объект с прецедентом
    Estimate Apply(Object const& precedent, Object const& o) const;

    // Сравнить объект с прецедентами
    Estimate Apply(ObjectPtrList const& precedents, Object const& o) const;
};





