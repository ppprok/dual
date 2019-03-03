#pragma once

#include <string>
#include <exception>

#include <boost/exception/all.hpp>
#include <boost/function.hpp>

struct error 
    : virtual std::exception, virtual boost::exception 
{	
};

typedef boost::error_info<struct tag_message, std::string> message;
typedef boost::error_info<struct tag_arg_name, std::string> arg_name;
typedef boost::error_info<struct tag_object_name, std::string> object_name;
typedef boost::error_info<struct tag_factory_name, std::string> factory_name;
typedef boost::error_info<struct tag_option_name, std::string> option_name;
typedef boost::error_info<struct tag_column_name, std::string> column_name;
typedef boost::error_info<struct tag_row_id, int> row_id;
typedef boost::error_info<struct tag_column_id, int> column_id;
typedef boost::error_info<struct tag_size, int> size;
typedef boost::error_info<struct tag_value_string, std::string> value_string;
typedef boost::error_info<struct tag_format, std::string> format;