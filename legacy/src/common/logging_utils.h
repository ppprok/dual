#pragma once

#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/trivial.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;

typedef logging::trivial::severity_level def_severity_level;
typedef src::severity_logger<def_severity_level> def_severity_logger;

def_severity_logger global_logger_init_impl();

BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(utils_logger, def_severity_logger) {
    return global_logger_init_impl();
}

//! Log catched exception diagnostic information
//void log_catched();

//! Add file for log output
void add_log_file(std::string const& filename);

//! Log info messages to std::cout
void add_cout_log();

//! Log warnings and errors to std::clog
void add_clog_log();

#define LOG_SEV(sev) BOOST_LOG_SEV(utils_logger::get(), def_severity_level::sev)
