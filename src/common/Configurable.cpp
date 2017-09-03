#include "stdafx.h"

#include <filesystem>

#include <boost/exception/errinfo_file_name.hpp>
#include "errors.h"

#include "Configurable.h"


Options read_json_options(std::string const& filename)
{
    Options opt;
    boost::property_tree::read_json(filename.c_str(), opt);
    return opt;
}

Options read_info_options(std::string const& filename)
{
    Options opt;
    boost::property_tree::read_info(filename.c_str(), opt);
    return opt;
}


Options read_options(std::string const& filename)
{
    std::tr2::sys::path path(filename);
    auto ext = std::tr2::sys::extension(path);
    Options opt;
            
    if (stricmp(ext.c_str(), ".json") == 0)
        boost::property_tree::read_json(filename.c_str(), opt);
    else if (stricmp(ext.c_str(), ".info") == 0)
        boost::property_tree::read_info(filename.c_str(), opt);
    else if (stricmp(ext.c_str(), ".ini") == 0)
        boost::property_tree::read_ini(filename.c_str(), opt);
    else if (stricmp(ext.c_str(), ".xml") == 0)
        boost::property_tree::read_xml(filename.c_str(), opt);
    else
        BOOST_THROW_EXCEPTION(file_format_error() << 
            format(ext) << boost::errinfo_file_name(filename));

    return opt;
}
