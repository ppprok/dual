
#include <string.h>
#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

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
    boost::filesystem::path path(filename);
    auto ext = path.extension();
    Options opt;

    if (strcasecmp(ext.c_str(), ".json") == 0)
        boost::property_tree::read_json(filename.c_str(), opt);
    else if (strcasecmp(ext.c_str(), ".info") == 0)
        boost::property_tree::read_info(filename.c_str(), opt);
    else if (strcasecmp(ext.c_str(), ".ini") == 0)
        boost::property_tree::read_ini(filename.c_str(), opt);
    else if (strcasecmp(ext.c_str(), ".xml") == 0)
        boost::property_tree::read_xml(filename.c_str(), opt);
    else
        BOOST_THROW_EXCEPTION(file_format_error() <<
                                                  boost::errinfo_file_name(filename));

    return opt;
}
