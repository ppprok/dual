#pragma once

#include "Interfaces.h"
#include "Object.h"


// Голосование по эл.кл.
class ECVoting
    :public IBinaryClassifier, public IECVoting
{
    // Эл.кл. с весами
    ECSet _ecset;

public:

    ECVoting(ECSet ecset)
        :_ecset(std::move(ecset))
    {
    }
    
    // Посчитать суммарный вес эл.кл., отдающих голос за отнесение объекта к
    // положительному классу
    virtual Estimate Apply( Object const& o ) const;

    ECSet const& GetECSet() const
    {
        return _ecset;
    }

    virtual void GetECSet( ECSet& ecset );
};