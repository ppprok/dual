#include "stdafx.h"

#include "Object.h"
#include "EC.h"


/*void Object::Apply( ECSet const& ecset, Positions& positions ) const
{
    positions.clear();
    int n = ecset.size();
    for(int i = 0; i < n; ++i)
        if (ecset[i].Apply(*this))
            positions.push_back(i);
}*/

int Object::Similarity(Object const& o) const
{
    int n1 = values.size();
    int n2 = o.values.size();
    int n = 0, d = 0;

    for (int j = 0; j < n1 && j < n2; ++j)
        if (values[j] == o.values[j])
            ++d;        
    return d;
}

int Object::HammingDistance(Object const& o) const
{
    int n1 = values.size();
    int n2 = o.values.size();
    int n = 0, d = 0;
    if (n1 > n2)
    {
        d = n1 - n2;
        n = n2;            
    }
    else
    {
        d = n2 - n1;
        n = n1;
    }

    for (int j = 0; j < n; ++j)
        if (values[j] != o.values[j])
            ++d;   

    return d;
}
