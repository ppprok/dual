#include "stdafx.h"

#include <stdio.h>

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_handle.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>

//#include "Exceptions.h"
#include "IntegersFileWriter.h"
#include "errors.h"

void IntegersFileWriter::CreateOutputFile( std::string const& outputFileName )
{
    _output = create_file_for_write(outputFileName);
}

void IntegersFileWriter::WriteInteger( int i, int pad)
{
    if (i < 0)
    {
        WriteChar('-');
        i = -i;
    }

    if (i < 10)
    {
        for(;pad > 1; --pad)
            WriteChar('0');
        
        WriteDigit(i);
        return;
    }

    if (i < 100)
    {
        for(;pad > 2; --pad)
            WriteChar('0');

        WriteDigit(i / 10);
        WriteDigit(i % 10);
        return;
    }

    WriteInteger(i / 100, pad - 2);
    WriteInteger(i % 100, 2);
}

void IntegersFileWriter::WriteBitNumbers( bit_chunk bits )
{
    int j = bits.find_first();
    if (j < 0)
        return;
    for (; ; )
    {
        WriteInteger(j);
        if ((j = bits.find_next(j)) < 0)
            break;
        WriteChar(' ');
    }
}

void IntegersFileWriter::WriteBitMatrix( bit_chunks const& rows )
{
    auto beg = rows.begin();
    auto end = rows.end();

    if (beg == end)
        return;

    for (;;)
    {
        WriteBitNumbers(*beg);
        if (++beg == end)
            break;
        WriteEOL();
    }
}


void IntegersFileWriter::WriteBitMatrix( bit_matrix const& bm )
{
    auto beg = bm.get_rows().begin();
    auto end = bm.get_rows().end();

    if (beg == end)
        return;

    for (;;)
    {
        WriteBitNumbers(*beg);
        if (++beg == end)
            break;
        WriteEOL();
    }
}

void IntegersFileWriter::CloseOutputFile()
{
    _output.reset();
}

IntegersFileWriter::IntegersFileWriter()
{
}

IntegersFileWriter::IntegersFileWriter(std::string const& filename)
{
    CreateOutputFile(filename);
}

void IntegersFileWriter::WriteNumber( double v, int precision /*= 3*/, int pad /*= 0*/)
{
    if (v < 0)
    {
        WriteChar('-');
        WriteNumber(-v, precision);
        return;
    }

    double f = floor(v);
    WriteInteger((int)f, pad);
    if (f < v)
    {
        WriteChar('.');
        WriteInteger((int)((v - f) * pow(10, precision)), precision);
    }
    
}