
#include "Predictions.h"
#include "containers.h"




int Prediction::LabelByEstimates() const
{
    if (predictions.empty())
        return -1;

    int n = predictions.size();
    if (n == 1)
        return predictions.front().label;

    
    int p1 = 0;
    Estimate me = predictions[0].estimate;

    int p2 = -1;
    
    for (int i = 1; i != n; ++i)
    {
        if (predictions[i].estimate > me)
        {
            me = predictions[i].estimate;
            p1 = i;
            p2 = -1;
        }
        else if (predictions[i].estimate == me)
        {
            p2 = i;
        }
    }

    if (p2 != -1)
        return -1;
    
    return predictions[p1].label;
}

ClassPrediction& Prediction::GetByLabel( int l )
{
    int p = PosByLabel(l);
    if (p == -1)
    {
        p = predictions.size();
        predictions.push_back(ClassPrediction(l));
    }

    return predictions[p];
}

Estimate Prediction::EstimateByLabel( int l ) const
{
    int pos = PosByLabel(l);
    if (pos < 0)
        return 0;
    return predictions[pos].estimate;
}

int Prediction::PosByLabel( int l ) const
{
    for (int p = 0, n = (int)predictions.size(); p != n ; ++p)
    {
        if (predictions[p].label == l)
            return p;
    }
    return -1;
}

Prediction::Prediction( ClassPredictionsMap const& p, int l /*= -1*/ )
{
    label = l;
    predictions.reserve(p.size());
    for(auto& kv:p)
    {
        predictions.emplace_back(kv.second);
    }
}

Prediction::Prediction( int l /*= -1*/ ) :label(l)
{

}

Estimate GroupedPredictions::GetMeanErrorIgnoreRejections()
{
    Estimate ce = 0;
    for (auto& p:groups)
    {
        Estimate e = 0;
        for (auto& pp:p)
        {
            int label = pp.LabelByEstimates();
            if (label >= 0 && pp.label != label)
                e+=1;
        }
        ce += e / p.size();
    }
    return ce / groups.size();
}

Estimate GroupedPredictions::GetMeanError() const
{
    Estimate ce = 0;
    Estimate f = 0;
    for (auto& p:groups)
    {
        for (auto& pp:p)
        {
            if (pp.label != pp.LabelByEstimates())
                ++ce;
            ++f;
        }            
    }
    return ce / f;
}

GroupedPredictions::GroupedPredictions( Predictions predictions )
{
    groups.emplace_back(std::move(predictions));
}

GroupedPredictions::GroupedPredictions()
{

}