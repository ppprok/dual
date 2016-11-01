#include "stdafx.h"

#include "DualizationBacktrackAlgorithmBase.h"
#include "IntegersFileReader.h"


void DualizationBacktrackAlgorithmBase::LoadMatrix( std::string const& filename )
{
    SetMatrix(IntegersFileReader(filename).ReadBitMatrix());
}

void DualizationBacktrackAlgorithmBase::DoInnerCallback()
{
    if (InnerCallback)
        InnerCallback->Call(_currentNode);
}
                                                                   
bool DualizationBacktrackAlgorithmBase::DoCoverCallback()
{
	_currentNode.type = NodeType::CoverNode;
	if (CoverCallback)
		CoverCallback->Call(_currentNode);
	return ! _currentNode.stoped;
}

bool DualizationBacktrackAlgorithmBase::AddColumn( int j )
{
    _currentNode.AddColumn(j);
	DoInnerCallback();
	return ! _currentNode.stoped && ! _currentNode.pruned;
}

void DualizationBacktrackAlgorithmBase::Backtrack()
{
    _currentNode.Backtrack();
    DoInnerCallback();
}

void DualizationBacktrackAlgorithmBase::ResetCurrentNode( int n /*= 0*/ )
{
    _currentNode.Reset(n);
}

void DualizationBacktrackAlgorithmBase::SetRestrictions( bit_matrix const& bm )
{
    int n = bm.width();
    _restrictions.zero(n, n);
    for(auto r: bm.get_rows())
    {
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
        {
            bit_chunk row = _restrictions.get_row(j);
            row.assign_or(row, r);
        }
    }    
}

void DualizationBacktrackAlgorithmBase::LoadRestrictions( std::string const& filename )
{
    SetRestrictions(IntegersFileReader(filename).ReadBitMatrix());
}

void DualizationBacktrackAlgorithmBase::SetMatrix( bit_matrix matrix )
{
    ResetCurrentNode(matrix.width());
    _matrix = std::move(matrix);
}

DualizationBacktrackAlgorithmBase::DualizationBacktrackAlgorithmBase() 
    :InnerCallback(0), CoverCallback(0)
{
}