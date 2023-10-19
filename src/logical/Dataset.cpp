
#include <fstream>
#include <set>


#include "Dataset.h"
#include "Interfaces.h"
#include "Randomizer.h"
#include "containers.h"
#include "IntegersFileReader.h"

ObjectsClasses const& Dataset::CollectClasses()
{
    std::set<int> labels;

    for(auto o : objects)
    {
        labels.insert(o->label);
    }

    classes.resize(labels.size());

    int i = 0;
    for (int l : labels)
    {
        classes[i].label = l;
        classes[i].positive.clear();
        classes[i].negative.clear();
        for(auto o : objects)
        {
            if (o->label == l)
                classes[i].positive.push_back(o);
            else
                classes[i].negative.push_back(o);
        }
        ++i;
    }

    return classes;
}

bool Dataset::Load( std::string const& filename )
{
    
    IntegersFileReader reader;
    reader.OpenInputFile(filename);
    
    std::vector<Values> v;
    reader.ReadUnsignedIntegersMatrix(v);
    
    /*std::ifstream in;
    in.open(filename.c_str());
    if (! in.is_open())
        return false;

    in.unsetf(std::ios::skipws);

    boost::spirit::istream_iterator inputIterator(in);

    static auto eoil = qi::eol|qi::eoi;
    static auto space = qi::space - eoil;
    static auto parser = (*qi::int_) % qi::eol;
    //static auto skiper =  
    //    (*space >> '#' >> *(qi::char_ - eoil) >> eoil)
    //    | space;
    static boost::spirit::istream_iterator end;

    

    bool r = qi::phrase_parse(inputIterator, end, parser, space, v);

    if (! r)
        return false;*/

    objects.resize(v.size());

    int bad = 0;
    int n = 0;

    for (int i = 0, s = v.size(); i < s; ++i)
    {
        if (v[i].empty())
        {
            objects[i].reset();
            ++bad;
            continue;
        }

        if (! objects[i])
            objects[i].reset(new Object());

        objects[i]->label = v[i].front();
        objects[i]->values = std::move(v[i]);
        objects[i]->values.erase(objects[i]->values.begin());

        if (n < (int)objects[i]->values.size())
            n = objects[i]->values.size();
    }

    if (bad > 0)
        objects.erase(std::remove_if(objects.begin(), objects.end(), [](ObjectPtr o){return ! o;}), objects.end());

    RemoveUncorrect();
    
    CollectClasses();

    LOG_SEV(info) << "Dataset loaded: filename=" << filename << 
        "; m=" << objects.size() << 
        "; l=" << classes.size() <<
        "; n=" << n;
    
    return true;
}




void Dataset::RandomShuffle()
{
	for (ObjectsClass& c:classes)
	{
		random_shuffle(c.positive);
        random_shuffle(c.negative);
	}

    random_shuffle(objects);
}


FeatureValues const& Dataset::AddFeatureValues(ObjectsClasses const& classes)
{
    for (auto& c : classes)
        AddFeatureValues(c.positive);
        
    return featureValues;
}

FeatureValues const& Dataset::AddFeatureValues( ObjectPtrList const& objects )
{
    if (objects.empty())
        return featureValues;

    int n = objects.front()->values.size();
    featureValues.reserve(featureValues.size() + n);

    for (auto& o : objects)
        AddFeatureValues(*o);

    return featureValues;
}

void Dataset::AddFeatureValues( Object const& o )
{
    for (int j = 0, n = o.values.size(); j < n; ++j)
        featureValues.emplace_back(j, o.values[j]);

    unique(featureValues);    
}

FeatureValues const& Dataset::CollectFeatureValues()
{
    featureValues.clear();
    if (classes.empty())
        return AddFeatureValues(objects);
    else
        return AddFeatureValues(classes);
}

ObjectsClass const& Dataset::GetClass(int label) const
{
    for (auto& c:classes)    
    {
        if (c.label == label)
            return c;
    }

    static ObjectsClass empty;
    empty.label = -1;
    return empty;
}

void Dataset::OrderByWeight(ObjectPtrList& objects) const
{
    std::sort(objects.begin(), objects.end(), 
        [](ObjectPtr const& o1, ObjectPtr const& o2)
    {
        if (! o1 && ! o2)
            return false;

        if (! o2)
            return true;

        if (! o1)
            return false;

        return o1->weight < o2->weight;});
}


void Dataset::OrderByWeightDescent(ObjectPtrList& objects) const
{
	std::sort(objects.begin(), objects.end(), 
		[](ObjectPtr const& o1, ObjectPtr const& o2)
	{
		if (! o1 && ! o2)
			return false;

		if (! o2)
			return true;

		if (! o1)
			return false;

		return o1->weight > o2->weight;});
}

void Dataset::Clear()
{
    classes.clear();
    objects.clear();
    featureValues.clear();
}



void Dataset::TakeRange( float from, float to, ObjectPtrList& output )
{
    if (! classes.empty())
    {
        for (auto& c : classes)
            TakeRange(c.positive, from, to, output);
    }
    else
    {
        TakeRange(objects, from, to, output);
    }
}

void Dataset::TakeRange( ObjectsClasses const& classes, float from, float to, ObjectPtrList& output )
{
    for (auto& c : classes)
        TakeRange(c.positive, from, to, output);
}

void Dataset::TakeRange( ObjectPtrList const& objects, float from, float to, ObjectPtrList& output )
{
    if (from < 0)
        from = 0;

    if (to > 1)
        to = 1;

    if (from >= to)
        return;
    int n = objects.size();
    for (int i = (int)floor(n * from), m = (int)ceil(n * to); i < m; ++i)
        output.push_back(objects[i]);
}

std::pair<Dataset, Dataset> Dataset::Split( float p1, float p2 ) const
{
    Dataset ds1;
    Dataset ds2;

    float e = p1 / (p1+p2);

    if (classes.empty())
    {
        TakeRange(objects, 0, e, ds1.objects);
        TakeRange(objects, e, 1, ds2.objects);
    }
    else
    {
        TakeRange(classes, 0, e, ds1.objects);
        TakeRange(classes, e, 1, ds2.objects);
    }

    return std::make_pair(std::move(ds1),std::move(ds2));
}

bool Dataset::RemoveUncorrect()
{
    if (objects.empty())
        return false;

    sort(objects, [](ObjectPtr const& o1, ObjectPtr const& o2)
    {
        if (equal(o1->values, o2->values))
            return o1->label < o2->label;

        return less(o1->values, o2->values);
    });

    int z = 0;
    for (int i = 0, m = objects.size(); i < m;)
    {
        bool uncorr = false;

        int j = i+1;
        for (; j < m; ++j)
        {
            if (! equal(objects[i]->values, 
                objects[j]->values)) break;

            if (objects[i]->label != objects[j]->label)
                uncorr = true;
        }

        for (int k = uncorr ? i : i+1; k < j; ++k)
        {
            objects[k]->values.clear();
            ++z;
        }

        i = j;
    }

    if (z)
    {
        LOG_SEV(warning) << "Removed "  << z << " uncorrect objects";

        pop_if(objects, [](ObjectPtr const& o){return o->values.empty();});
        return true;
    }

    return false;
}

void Dataset::RestoreLabels()
{
    RestoreLabels(labels);
}

void Dataset::RestoreLabels( Labels const& labels )
{
    assert(objects.size() == labels.size());

    for (int i = 0, m = labels.size(); i != m; ++i)
        objects[i]->label = labels[i];
}

Labels& Dataset::StoreLabels()
{
    int m = objects.size();
    labels.resize(m);
    for (int i = 0; i != m; ++i)
        labels[i] = objects[i]->label;

    return labels;
}

void Dataset::RestoreWeights()
{
    RestoreWeights(weights);
}

void Dataset::RestoreWeights( Weights const& weights )
{
    assert(objects.size() == weights.size());

    for (int i = 0, m = weights.size(); i != m; ++i)
        objects[i]->weight = weights[i];
}

Weights& Dataset::StoreWeights()
{
    int m = objects.size();
    weights.resize(m);
    for (int i = 0; i != m; ++i)
        weights[i] = objects[i]->weight;

    return weights;
}

Dataset::Dataset()
{

}