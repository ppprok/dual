
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace logging = boost::log;

#include "logging_utils.h"

void add_log_file(std::string const& filename) {
    //    logging::aux::add_file_sink
    //        (
    //        keywords::file_name = filename,                                        /*< file name pattern >*/
    //        keywords::format = (
    //        expr::stream
    //        << expr::format_named_scope("Scopes", keywords::format = "%n")
    //        << " [" << expr::attr<logging::trivial::severity_level>("Severity")
    //        << "] " << expr::message
    //        )
    //        );
}

void add_cout_log() {
    //    logging::add_console_log(std::cout,
    //        keywords::filter=
    //        (
    //        expr::attr<def_severity_level>("Severity") == def_severity_level::info
    //        ),
    //        keywords::format =
    //        (
    //        expr::stream << expr::message
    //        ));
}

void add_clog_log() {
    //    logging::aux::add_console_log(std::clog,
    //        keywords::filter=
    //        (
    //        expr::attr<def_severity_level>("Severity") >= def_severity_level::warning
    //        ),
    //        keywords::format = (
    //        expr::stream
    //        << expr::format_named_scope("Scopes", keywords::format = "%n")
    //        << " [" << expr::attr<logging::trivial::severity_level>("Severity")
    //        << "] " << expr::message
    //        )
    //        );
}

def_severity_logger global_logger_init_impl() {
    def_severity_logger lg(keywords::severity = def_severity_level::info);
    lg.add_attribute("Scopes", attrs::named_scope());
    lg.add_attribute("TimeStamp", attrs::local_clock());
    return lg;
}
