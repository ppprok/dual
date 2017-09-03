#include "stdafx.h"
#include "ECVoting.h"



Estimate ECVoting::Apply( Object const& o ) const
{
    Estimate e = 0;
    Estimate negw = 0;
    Estimate posw = 0;
    //Estimate negw0 = 0;
    //Estimate posw0 = 0;
    
    for (auto& ec:_ecset)
    {
        auto w = ec.weight;
        if (! w)
            continue;
        
        if (w < 0)
        {
            negw -= w;
            //if (negw0 > w)
              //  negw0 = w;
        }
        else 
        {
            posw += w;
            //if (posw0 < w)
              //  posw0 = w;
        }
                
        if (ec.Apply(o))
            e += w;
    }

    if (! e)
        return 0;
    
    if (e < 0)
    {
        //if (e <= negw0)
          //  return -1;
        return e / negw;
    }
    else
    {
        //if (e >= posw0)
            //return 1;
        return e / posw;
    }
}

void ECVoting::GetECSet( ECSet& ecset )
{
    ecset.insert(ecset.end(), _ecset.begin(), _ecset.end());
}