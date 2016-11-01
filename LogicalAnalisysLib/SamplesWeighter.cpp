#include "stdafx.h"

#include "SamplesWeighter.h"

Weight SamplesWeighter::Delta(Weight posw, Weight negw, Weight epsilon)
{
    return sqr(sqrt(posw + epsilon) - sqrt(negw + epsilon)) - 2 * epsilon;
}

SelectedClassInfo SamplesWeighter::SelectPositiveNegativeSamples(Weight epsilon, Weight delta, ObjectPtrList& positive, ObjectPtrList& negative)
{
    auto cls = SelectRandomClass(Delta(delta + 3*epsilon/2, epsilon / 2, epsilon / 2));

    if (cls.weight < 0)
        cls = SelectRandomClass(0);
    
    if (cls.weight < 0)
        return SelectedClassInfo(-1, -1, false);
    
    OrderSamplesDescend();
    PartitionByClass(cls.label, positive, negative);
    if (cls.negative)
    {
        positive.swap(negative);
    }

    Weight posw = cls.weight;
    Weight negw = 0;

    for(;;)
    {
        Weight posdelta = (positive.size() > 1) ? Delta(posw - positive.back()->weight, negw, epsilon) : 0;
        Weight negdelta = (negative.size() > 1) ? Delta(posw, negw + negative.back()->weight, epsilon) : 0;

        if (posdelta > delta && negdelta > delta)
        {
            if (negative.size() > positive.size() || (negative.size() == positive.size() && positive.back()->weight < negative.back()->weight))
            {
                posw -= positive.back()->weight;
                positive.pop_back();
                continue;
            }
            else
            {
                negw += negative.back()->weight;
                negative.pop_back();
                continue;
            }
        }
        else
        {
            if (posdelta> delta)
            {
                posw -= positive.back()->weight;
                positive.pop_back();
                continue;
            }
            else if (negdelta > delta)
            {
                negw += negative.back()->weight;
                negative.pop_back();
                continue;
            }
        }

        break;
    }

    return cls;
}

void SamplesWeighter::PartitionByClass(int label, ObjectPtrList& positive, ObjectPtrList& negative)
{
    positive.clear();
    negative.clear();

    for (auto& o:_objects)
    {
        if (! o)
            continue;
        if (o->label == label)
            positive.push_back(o);
        else
            negative.push_back(o);
    }
}

SelectedClassInfo SamplesWeighter::SelectRandomClass(Weight delta)
{
    ClassWeightMap positive;
    ClassWeightMap negative;

    Weight s = 0;

    for (auto c:_classWeights)
    {
        if (c.second > delta)
            s += (positive[c.first] = c.second);

        Weight negWeight = _sumWeight - c.second;
        if (negWeight > delta)
            s += (negative[c.first] = negWeight);
    }

    Weight r = _rand() * s;
    Weight t = 0;

    for (auto p:positive)
    {
        t += p.second;
        if (t >= r)
            return SelectedClassInfo(p.first, p.second, false);
    }

    for (auto p:negative)
    {
        t += p.second;
        if (t >= r)
            return SelectedClassInfo(p.first, p.second, true);
    }

    return SelectedClassInfo(-1, -1, false);
}

void SamplesWeighter::UpdateClassWeights()
{
    OnUpdateWeights();
}

void SamplesWeighter::RemoveNull()
{
    remove_false(_objects);
}

void SamplesWeighter::OrderSamplesDescend()
{
    std::sort(_objects.begin(), _objects.end(),
        [](ObjectPtr const& o1, ObjectPtr const& o2)
    {
        if (! o1)
            return false;
        if (! o2)
            return true;
        return o1->weight > o2->weight;
    });
}

void SamplesWeighter::OrderSamples()
{
    std::sort(_objects.begin(), _objects.end(),
        [](ObjectPtr const& o1, ObjectPtr const& o2)
    {
        if (! o1)
            return false;
        if (! o2)
            return true;
        return o1->weight < o2->weight;
    });
}

ObjectPtr SamplesWeighter::GetRandomSample() const
{
    Weight r = _rand() * _sumWeight;
    int m = _objects.size();

    Weight s1 = 0;
    Weight s2 = _sumWeight;
    for (int i = 0, j = m-1;  i <= j; ++i, --j)
    {
        if (_objects[i])
        {
            s1 += _objects[i]->weight;
            if (s1 >= r)
                return _objects[i];
        }

        if (_objects[j])
        {
            s2 -= _objects[j]->weight;
            if (s2 <= r)
                return _objects[j];
        }
    }

    assert(false);

    return ObjectPtr();
}

ObjectPtrList SamplesWeighter::GetRandomSamples(int n) const
{
    ObjectPtrList samples;
    if (n <=0)
        return samples;

    samples.reserve(n);

    Weights rnd(n, 0);

    std::generate(rnd.begin(), rnd.end(),
        [this](){return _rand()*_sumWeight;});

    sort(rnd);

    Weight s = _sumWeight;
    for (int i = _objects.size()-1; 
        i >=0 && ! rnd.empty(); --i)
    {
        if (! _objects[i])
            continue;

        s -= _objects[i]->weight;
        while (! rnd.empty() && s < rnd.back())
        {
            rnd.pop_back();
            samples.push_back(_objects[i]);
        }

    }

    std::random_shuffle(samples.begin(), samples.end());
    return std::move(samples);
}

ObjectPtrList SamplesWeighter::GetRandomSamples(int n, SelectedClassInfo const& cls) const
{
    ObjectPtrList samples;
    if (n <=0)
        return samples;

    samples.reserve(n);

    Weights rnd(n, 0);

    std::generate(rnd.begin(), rnd.end(),
        [this, &cls](){return _rand()*cls.weight;});

    sort(rnd);

    Weight s = cls.weight;
    for (int i = _objects.size()-1; 
        i >=0 && ! rnd.empty(); --i)
    {
        if (! _objects[i])
            continue;

        if (cls.negative && _objects[i]->label == cls.label)
            continue;

        if (! cls.negative && _objects[i]->label != cls.label)
            continue;

        s -= _objects[i]->weight;
        while (! rnd.empty() && s < rnd.back())
        {
            samples.push_back(_objects[i]);
            rnd.pop_back();
        }
    }

    std::random_shuffle(samples.begin(), samples.end());

    return std::move(samples);
}

void SamplesWeighter::RandomShuffle()
{
    std::random_shuffle(_objects.begin(), _objects.end());
}

void SamplesWeighter::NormalizeWeights()
{
    if (_sumWeight == 0)
        UpdateWeightSum();

    Weight s = _sumWeight;

    if (s == 0)
        return;
        
    _sumWeight = 0;
    _classWeights.clear();

    for(auto& o:_objects)
    {
        if (! o)
            continue;
        o->weight /= s;
        OnUpdateWeight(o);
    }
}

Weight SamplesWeighter::GetWeightSum() const
{
    return _sumWeight;
}

void SamplesWeighter::SetWeight(Weight w)
{
    for(auto o:_objects)
    {
        if (! o)
            continue;
        o->weight = w;
    }        
    OnUpdateWeights();
}

ObjectPtrList const& SamplesWeighter::GetObjects() const
{
    return _objects;
}

void SamplesWeighter::UpdateWeightSum()
{
    _sumWeight = 0;
    for (auto& o:_objects)
        if (o)
            _sumWeight += o->weight;
}

void SamplesWeighter::SetObjects(ObjectPtrList objects)
{
    _objects = std::move(objects);
    OnUpdateWeights();
}

SamplesWeighter::ClassWeightMap const& SamplesWeighter::GetClassWeights() const
{
    return _classWeights;
}

void SamplesWeighter::OnUpdateWeights()
{
    _sumWeight = 0;
    _classWeights.clear();
    OnUpdateWeights(_objects);
}

void SamplesWeighter::OnUpdateWeights(ObjectPtrList const& objects)
{
    for (auto& o : objects)
        OnUpdateWeight(o);
}

void SamplesWeighter::OnUpdateWeight(ObjectPtr const& o)
{
    if (! o)
        return;
    Weight w = o->weight;
    _sumWeight += w;
    _classWeights[o->label] += w;
}
