
#include "FileWriters.h"
#include "DualizationBacktrackAlgorithmBase.h"

void AllColumnsFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        WriteIntegers(node.columns.begin(), node.columns.end());
        WriteEOL();
    }
}

void OrderedColumnsFileWriterCallback::Call( DualizationNode& node )
{
    if (! node.IsCover())
		return;

    _buffer.resize_raw(node.columns.size());
    std::copy(node.columns.begin(), node.columns.end(), _buffer.begin());
    std::sort(_buffer.begin(), _buffer.end());
    WriteIntegers(_buffer.begin(), _buffer.end());

    WriteEOL();
}

void FirstColumnFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        if (! node.columns.empty())
            WriteInteger(node.columns.front());
        WriteEOL();
    }
}

void LastColumnFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        if (! node.columns.empty())
            WriteInteger(node.columns.back());
        WriteEOL();
    }
}

void SizeFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        WriteInteger(node.columns.size());
        WriteEOL();
    }
}

void MinColumnFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        if (! node.columns.empty())
            WriteInteger(*std::min_element(node.columns.begin(), node.columns.end()));
        WriteEOL();
    }
}

void MaxColumnFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        if (! node.columns.empty())
            WriteInteger(*std::max_element(node.columns.begin(), node.columns.end()));
        WriteEOL();
    }
}

void WeightFileWriterCallback::Call( DualizationNode& node )
{
    if (node.IsCover())
    {
        WriteNumber(node.weight, 5);
        WriteEOL();
    }
}