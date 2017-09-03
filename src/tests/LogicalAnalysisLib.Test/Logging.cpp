#include "stdafx.h"
#include "CppUnitTest.h"
#include "logging_utils.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace LogicalAnalysisLibTest
{
	TEST_MODULE_INITIALIZE(Logging)
	{
        // Создаём журнал
        logging::add_file_log
            (
            keywords::file_name = "LALib.Test.log",                                       /*< file name pattern >*/
            //keywords::rotation_size = 10 * 1024 * 1024,                                   /*< rotate files every 10 MiB... >*/
            //keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0), /*< ...or at midnight >*/
            keywords::format = (
            expr::stream
            << expr::format_named_scope("Scopes", keywords::format = "%n") 
            << " [" << expr::attr<def_severity_level>("Severity") << "] " << expr::message 
            )
            );

        logging::core::get()->set_filter
            (
            // Выводим только важные и информационные сообщения    
            expr::attr<def_severity_level>("Severity") >= def_severity_level::trace
            );
	};
}