#include "stdafx.h"

#include "CmpMatrixBuilder.h"
#include <set>
#include "Exceptions.h"




int CmpMatrixBuilder::MakeObjectPositionMap( ObjectPtrList const& objects, ObjectPositionMap& posMap )
{
    int m = 0;
    posMap.clear();
    for (auto& o:objects)
    {
        if (GetObjectPosition(posMap, o) == -1)
            posMap[o.get()] = m++;
    }
    return m;
}

int CmpMatrixBuilder::MakeOptionalObjectPositionMap( 
    ObjectPtrList const& optional, 
    ObjectPtrList const& objects, 
    ObjectPositionMap& posMap, 
    ObjectPtrList& registered)
{
    int m = 0;
    posMap.clear();
    registered.clear();
    registered.reserve(optional.size());
    
    for (auto& o:optional)
    {
        if (GetObjectPosition(posMap, o) == -1)
        {
            if (std::find(objects.begin(), objects.end(), o) != objects.end())
            {
                posMap[o.get()] = m++;
                registered.push_back(o);
            }
        }
    }
    return m;
}

int CmpMatrixBuilder::GetObjectPosition( ObjectPositionMap const& posMap, ObjectPtr const& o )
{
    auto i = posMap.find(o.get());
    if (i == posMap.end())
        return -1;
    return i->second;
}

int CmpMatrixBuilder::GetObjectsPositions( ObjectPositionMap const& posMap, ObjectPtrList const& objects, ObjectPtrList& present, Positions& positions )
{
    present.clear();
    present.reserve(std::min(objects.size(), posMap.size()));
    positions.clear();
    positions.reserve(present.capacity());

    for(auto& o: objects)
    {
        int i = GetObjectPosition(posMap, o);
        if (i == -1)
            continue;
        present.push_back(o);
        positions.push_back(i);
    }

    return positions.size();
}

bit_chunk CmpMatrixBuilder::GetRow( ObjectPtr const& pos, ObjectPtr const& neg )
{
    int i1 = GetObjectPosition(_positivePositions, pos);
    if (i1 == -1)
        return bit_chunk();
    int i2 = GetObjectPosition(_negativePositions, neg);
    if (i2 == -1)
        return bit_chunk();

    int m2 = _negativePositions.size();        
    return _cmpMatrix.get_row(i1 * m2 + i2);
}

int CmpMatrixBuilder::GetOptionalObjectPosition(ObjectPtr const& o, 
                                                ObjectPtrList const& optional,
                                                ObjectPositionMap& registeredPositions, 
                                                ObjectPtrList& registered)
{
    int pos = GetObjectPosition(registeredPositions, o);
    if (pos != -1)
        return pos;

    if (std::find(optional.begin(), optional.end(), o) != optional.end())
    {
        registered.push_back(o);
        return registeredPositions[o.get()] = registered.size()-1;            
    }
    
    return registeredPositions[o.get()] = -2;    
}

CmpMatrixSubmatrix CmpMatrixBuilder::GetSubmatrix(
    ObjectPtrList const& positive, 
    ObjectPtrList const& negative,
    ObjectPtrList const& optionalPositive,
    ObjectPtrList const& optionalNegative)
{
    Positions positivePositions;        
    Positions negativePositions;

    CmpMatrixSubmatrix submatrix;

    int r1 = GetObjectsPositions(_positivePositions, positive, submatrix.Positive, positivePositions);
    int r2 = GetObjectsPositions(_negativePositions, negative, submatrix.Negative, negativePositions);
    int m2 = _negativePositions.size();
    
    ObjectPositionMap optionalPositivePositions;
    int n1 = MakeOptionalObjectPositionMap(optionalPositive, 
        submatrix.Positive, 
        optionalPositivePositions, 
        submatrix.OptionalPositive);
        
    ObjectPositionMap optionalNegativePositions;
    int n2 = MakeOptionalObjectPositionMap(optionalNegative, 
        submatrix.Negative, 
        optionalNegativePositions, 
        submatrix.OptionalNegative);

    int n = _localBasis.size()+1;

    if (_reservedColumns < n1 + n2)
        Throw("Зарезервировано мало столбцов для опциональных объектов");
    
    submatrix.Rows.reserve(r1 * r2);

    for(int i =0; i < (int)positivePositions.size(); ++i)
    {
        int opos = GetObjectPosition(optionalPositivePositions, submatrix.Positive[i]);
        
        for (int j = 0; j < (int)negativePositions.size(); ++j)
        {
            int oneg = GetObjectPosition(optionalNegativePositions, submatrix.Negative[j]);
                        
            auto r = positivePositions[i] * m2 + negativePositions[j];
            
            // Заполнение резервных столбцов, предназначенных для опциональных объектов
            for (int k = n; k < n + _reservedColumns; ++k)
            {
                _cmpMatrix.set(r, k, (opos != -1 && k == n + opos) || (oneg != -1 && k == n + n1 + oneg));
            }

            submatrix.Rows.push_back(_cmpMatrix.get_row(r));
        }
    }

    return std::move(submatrix);
}

void CmpMatrixBuilder::Build( ObjectPtrList const& positive, ObjectPtrList const& negative, ECmpSet localBasis, int reserveColumns /*= 0*/ )
{
    assert(reserveColumns >= 0);
    _reservedColumns = reserveColumns;

    int m1 = MakeObjectPositionMap(positive, _positivePositions);
    int m2 = MakeObjectPositionMap(negative, _negativePositions);
    int n1 = localBasis.size();
    int m = m1 * m2;
    int n = n1 + reserveColumns + 1;

    _cmpMatrix.zero(m, n);

    for (auto pos:_positivePositions)
    {
        for(auto neg: _negativePositions)
        {
            int rownum = pos.second * m2 + neg.second;
            int nz = 0;
            for (int j = 0; j < n1; ++j)
            {
                if (! localBasis[j].Apply(*pos.first, *neg.first))
                {
                    _cmpMatrix.set(rownum, j);
                    ++nz;
                }
            }
            
            if (nz == 0)
            {
                // Один столбец для обозначения неразличимых объектов
                _cmpMatrix.set(rownum, n1);
            }
        }
    }

    _localBasis = std::move(localBasis);
}

ECmpSet CmpMatrixBuilder::GetECmpSubset( Positions const& positions )
{
    ECmpSet result;
    result.reserve(positions.size());

    for (auto p:positions)
    {
        if (inside(_localBasis, p))
            result.push_back(_localBasis[p]);
    }

    return std::move(result);
}

void CmpMatrixBuilder::GetExcludedOptionalObjects( CmpMatrixSubmatrix const& submatrix, Positions const& cover, ObjectPtrList& excludedPositive, ObjectPtrList& excludedNegative )
{
    excludedPositive.clear();
    excludedNegative.clear();

    for (auto p:cover)
    {
        p -= _localBasis.size()+1;
        if (p < 0)
            continue;

        if (inside(submatrix.OptionalPositive, p))
        {
            excludedPositive.push_back(submatrix.OptionalPositive[p]);
        }
        else
        {
            p -= submatrix.OptionalPositive.size();
            if (inside(submatrix.OptionalNegative, p))
                excludedNegative.push_back(submatrix.OptionalNegative[p]);
        }
    }
}