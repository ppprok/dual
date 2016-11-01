#include "stdafx.h"


#include "errors.h"
#include "io_utils.h"

std::shared_ptr<FILE> open_file_for_read(std::string const& filename)
{
	FILE* f = fopen(filename.c_str(), "rb"); 

	if(! f)
		BOOST_THROW_EXCEPTION(
		file_open_error() <<
		boost::errinfo_api_function("fopen") <<
		boost::errinfo_errno(errno) <<
		boost::errinfo_file_name(filename) <<
		boost::errinfo_file_open_mode("rb") );
	
	return std::shared_ptr<FILE>(f, fclose);
}

FILE_ptr create_file_for_write(std::string const& filename)
{
	FILE* f = fopen(filename.c_str(), "wb"); 

	if(! f)
		BOOST_THROW_EXCEPTION(
		file_create_error() <<
		boost::errinfo_api_function("fopen") <<
		boost::errinfo_errno(errno) <<
		boost::errinfo_file_name(filename) <<
		boost::errinfo_file_open_mode("wb") );

	return std::shared_ptr<FILE>(f, fclose);
}

blob<char> read_all(std::string const& filename)
{
	auto file = open_file_for_read(filename);

	auto size = _filelength(file->_file);

	if(size==-1L)
		BOOST_THROW_EXCEPTION(
		file_error() <<
		boost::errinfo_api_function("_filelength") <<
		boost::errinfo_errno(errno) <<
		boost::errinfo_file_name(filename) );

	
	blob<char> data(size);

	if (size)
	{
		if (_read(file->_file, data.begin(), size) != size)
			BOOST_THROW_EXCEPTION(
			file_read_error() <<
			boost::errinfo_api_function("_read") <<
			boost::errinfo_errno(errno) <<
			boost::errinfo_file_name(filename) );
	}

	return data;
}

void write( FILE* file, char const* begin, char const* end )
{
    assert(file);
    auto count = end - begin;
    if (count == 0)
        return;

    if (fwrite(reinterpret_cast<void*>(const_cast<char*>(begin)), 
        1, count, file) != count)
    {
        BOOST_THROW_EXCEPTION(
            file_write_error() <<
            boost::errinfo_api_function("fwrite") <<
            boost::errinfo_errno(errno));
    }
}

void write( FILE* file, std::string const& str)
{
    assert(file);
    auto count = str.length();
    if (count == 0)
        return;

    if (fwrite(reinterpret_cast<void*>(const_cast<char*>(&str[0])), 
        1, count, file) != count)
    {
        BOOST_THROW_EXCEPTION(
            file_write_error() <<
            boost::errinfo_api_function("fwrite") <<
            boost::errinfo_errno(errno));
    }
}

void write( FILE* file, char const* begin )
{
    assert(file);
    int r = 0;
    for (;*begin && r!= EOF;++begin)
        r = fputc(*begin, file);
    if (r == EOF)
    {
        BOOST_THROW_EXCEPTION(
            file_write_error() <<
            boost::errinfo_api_function("fputc") <<
            boost::errinfo_errno(errno));
    }
}
