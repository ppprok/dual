
#include "BoolBinaryOperation.h"

bit_chunk BoolBinaryOperation::Apply( bit_chunk o1, bit_chunk o2, bit_chunk temp, bit_chunk& r, int n )
{
    r.clear();

    if (Apply(false, false))
    {
        r.assign_or(o1, o2);
        r.flip();
        r.pad(n, true);
    }

    if (Apply(true, true))
    {
        if (r.empty())
        {
            r.assign_and(o1, o2);
        }
        else
        {
            temp.assign_and(o1, o2);
            r.assign_or(r, temp);
        }
    }

    if (Apply(true, false))
    {
        if (r.empty())
        {
            r.assign_sub(o1, o2);
        }
        else
        {
            temp.assign_sub(o1, o2);
            r.assign_or(r, temp);
        }
    }

    if (Apply(false, true))
    {
        if (r.empty())
        {
            r.assign_sub(o2, o1);
        }
        else
        {
            temp.assign_sub(o2, o1);
            r.assign_or(r, temp);
        }
    }

    return r;
}

BoolBinaryOperation BoolBinaryOperation::operator!() const
{
    BoolBinaryOperation o(*this);
    for(auto& v:o.table)
        v = !v;
    return o;
}

bool BoolBinaryOperation::operator==( BoolBinaryOperation& op ) const
{
    return table == op.table;
}

BoolBinaryOperation& BoolBinaryOperation::operator^=( BoolBinaryOperation const& o )
{
    for (int i = 0; i < 4; ++i)
        table[i] = table[i] ^ o.table[i];
    return *this;
}

BoolBinaryOperation& BoolBinaryOperation::operator|=( BoolBinaryOperation const& o )
{
    for (int i = 0; i < 4; ++i)
        table[i] = table[i] || o.table[i];
    return *this;
}

BoolBinaryOperation& BoolBinaryOperation::operator&=( BoolBinaryOperation const& o )
{
    for (int i = 0; i < 4; ++i)
        table[i] = table[i] && o.table[i];
    return *this;
}

std::vector<BoolBinaryOperation> BoolBinaryOperation::Split()
{
    std::vector<BoolBinaryOperation> operations;

    for (int i = 0; i < 4; ++i)
    {
        if (! table[i])
            continue;
        operations.emplace_back();
        operations.back().table[i] = true;
    }

    return std::move(operations);
}

bool BoolBinaryOperation::Apply( bool o1, bool o2 ) const
{
    return table[GetTableIndex(o1, o2)];
}

int BoolBinaryOperation::GetTableIndex( bool o1, bool o2 ) const
{
    int i = 0;
    if (o1)
    {
        ++i;
        ++i;
    }
    if (o2)
    {
        ++i;
    }
    return i;
}

void BoolBinaryOperation::Set( bool o1, bool o2, bool r )
{
    table[GetTableIndex(o1, o2)] = r;
}

BoolBinaryOperation::BoolBinaryOperation( bool constant /*= false*/ )
{
    table.assign(constant);
}