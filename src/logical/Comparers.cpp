#include "stdafx.h"

#include "Comparers.h"




void FeatureComparer::Get( bit_chunk cover, Features& selected )
{
    cover.get_selected(features, selected);
}

void FeatureComparer::Compare( Object const& o1, Object const& o2, bit_chunk& output) const
{
    output.set_if(features, [&o1,&o2](int j)
                                 {return o1.values[j] != o2.values[j];}
    );
}

int FeatureComparer::GetSize() const
{
    return features.size();
}

void OptionalObjects::Get( bit_chunk cover, ObjectPtrList& selected )
{
    cover.get_selected(optionalPrecedents, selected);
}

void OptionalObjects::Compare( Object const& o1, Object const& o2, bit_chunk& output) const
{
    output.set_if(optionalPrecedents, 
        [&o1](ObjectPtr const& o)
    {
        return &o1 == o.get();
    });
}

int OptionalObjects::GetSize() const
{
    return optionalPrecedents.size();
}


// Выровнять размер в битах
int align_size(int s)
{
    return bit_chunk::block_for_bits(s) << bit_chunk::low_index_bit_count;
}


int ComparersComposer::GetSize() const
{
    int s = 0;
    for (auto const& c : comparers)
    {
        if (! s)
            s = c->GetSize();
        else
            s = align_size(s) + c->GetSize();
    }

    return s;
}

void ComparersComposer::Compare( Object const& o1, Object const& o2, bit_chunk& output) const
{
    output.clear();
    
    auto r = output;    
    for (auto const& c : comparers)
    {
        int s = bit_chunk::block_for_bits(c->GetSize());
        r.pad_blocks(s);
        c->Compare(o1, o2, r);
        r.begin = r.end = r.begin + s;
    }

    output.end = r.end;
    output.trim();
}

bit_chunk ComparersComposer::GetPart( bit_chunk const& cover, int index ) const
{
    assert(inside(comparers, index));

    auto part = cover;

    for (int i = 0; i != index; ++i)
    {
        part.begin += bit_chunk::block_for_bits(comparers[i]->GetSize());
        if (part.begin >= part.end)
        {
            part.end = part.begin;
            return part;
        }
    }

    part.end = part.begin + bit_chunk::block_for_bits(comparers[index]->GetSize());
    if (part.end > cover.end)
        part.end = cover.end;

    return part;
}

int ECmpSetComparer::GetSize() const
{
    return ecmpSet.size();
}

void ECmpSetComparer::Compare( Object const& o1, Object const& o2, bit_chunk& output ) const
{
    output.set_if(ecmpSet, [&o1,&o2](ECmp const& ecmp)
        {return ! ecmp.Apply(o1, o2);});
}

void ECmpSetComparer::Get( bit_chunk cover, ECmpSet& selected )
{
    cover.get_selected(ecmpSet, selected);
}