#pragma once

#include "Object.h"
#include "ECmp.h"


// Предикат, основанный на сравнении откликов прецедентов и распознаваемого объекта 
// на наборе эл.кл.
class ECmpSetPredicate
{
    
public:

    ObjectPtrList precedents;
    ECmpSetConjunction ecmpSet;
    Weight weight;

    UTILITY_MOVE_DEFAULT_MEMBERS(ECmpSetPredicate, (precedents)(ecmpSet)(weight));

    ECmpSetPredicate(
        ObjectPtrList precedents_, 
        ECmpSetConjunction ecmpSet_, 
        Weight weight_ = 1)
        :precedents(precedents_), ecmpSet(std::move(ecmpSet_)), weight(weight_)
    {
    }
    
    // Применение предиката к распознаваемому объекту
    Weight Apply(Object const& obj, bool additive) const
    {
        Weight w = 0;
        for(auto& p:precedents)
        {
            if (! ecmpSet.Apply(*p, obj))
                continue;
            w += weight ? weight : 1;
            if (! additive)
                break;
        }

		if (w && additive)
			w/=precedents.size();

		return w;
    }

    // Применить к нескольким объектам и найти суммарный вес выделяемых объектов
    Weight ApplyMany(ObjectPtrList const& objects, bool additive) const
    {
        Weight w = 0;

        for (auto& o : objects)
        {
            w += o->weight * Apply(*o, additive);
        }

        return w;
    }

};


typedef std::vector<ECmpSetPredicate> ECmpSetPredicates;