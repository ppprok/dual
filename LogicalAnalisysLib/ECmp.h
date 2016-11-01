#pragma once

#include <utility>

#include "BoolBinaryOperation.h"
#include "EC.h"



//////////////////////////////////////////////////////////////////////////
// Сравнение объектов с помощью эл.кл. и логической операции
//////////////////////////////////////////////////////////////////////////
struct ECmp
{
    enum RelationType:byte
    {
        Or = 0,
        LE = 1,
        GE = 2,
        NotAnd = 3,
        EQ = 4
    };

    EC ec;

    RelationType op;

    UTILITY_MOVE_DEFAULT_MEMBERS(ECmp, (ec)(op));

    ECmp()
        :op(LE)
    {};
    
    ECmp(EC ec_, RelationType const& op_)
        :ec(std::move(ec_)),  op(op_)
    {};

    ECmp(EC ec_, RelationType const& op_, Weight w)
        :ec(std::move(ec_)),  op(op_)
    {
        ec.weight = w;
    };

    bool operator <(ECmp const& ecmp) const
    {
        if (op == ecmp.op)
            return ec < ecmp.ec;

        return op < ecmp.op;
    }

    bool operator ==(ECmp const& ecmp) const
    {
        return op == ecmp.op && ec == ecmp.ec;
    }
        
    bool Apply(bool r1, bool r2) const
    {
        switch (op)
        {
        case ECmp::Or:
            return r1 || r2;
        case ECmp::LE:
            return r1 <= r2;
        case ECmp::GE:
            return r1 >= r2;
        case ECmp::NotAnd:
            return ! (r1 && r2);
        case ECmp::EQ:
            return r1 == r2;
        }
        return true;
    }
    
    bool Apply(Object const& o1, bool r2) const
    {
        if (! r2)
        {
            switch (op)
            {
            case ECmp::Or:
            case ECmp::EQ:
                return ec.Apply(o1);
            case ECmp::LE:
                return ! ec.Apply(o1);
            }
        }
        else
        {
            switch (op)
            {
            case ECmp::EQ:
            case ECmp::NotAnd:
                return ! ec.Apply(o1);
            case ECmp::GE:
                return ec.Apply(o1);
            }
        }

        return true;
    }

    bool Apply(bool r1, Object const& o2) const
    {
        if (! r1)
        {
            switch (op)
            {
            case ECmp::Or:
            case ECmp::EQ:
                return ec.Apply(o2);
            case ECmp::GE:
                return ! ec.Apply(o2);
            }
        }
        else
        {
            switch (op)
            {
            case ECmp::EQ:
            case ECmp::NotAnd:
                return ! ec.Apply(o2);
            case ECmp::LE:
                return ec.Apply(o2);
            }
        }

        return true;
    }


    // Сравнение результатов применения эл.кл. к двум объектам
    bool Apply(Object const& o1, Object const& o2) const
    {
        return Apply(o1, ec.Apply(o2));
    }
};


typedef std::vector<ECmp> ECmpSet;

// Набор эл.кл. и их отношений
struct ECmpSetConjunction
{
    ECmpSet ecmpSet;

    UTILITY_MOVE_DEFAULT_MEMBERS(ECmpSetConjunction, (ecmpSet));

    ECmpSetConjunction()
    {
    }

    ECmpSetConjunction(ECmpSet ecmpSet_)
        :ecmpSet(ecmpSet_)
    {
    }

    // Сравнение откликов наборов эл.кл.
    bool Apply(Object const& o1, Object const& o2) const
    {
        for (auto& ecmp:ecmpSet)
        {
            if (! ecmp.Apply(o1, o2))
                return false;
        }
        return true;
    }
};