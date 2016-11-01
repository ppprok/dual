// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Headers for CppUnitTest
#include <CppUnitTest.h>

// TODO: reference additional headers your program requires here

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <tchar.h>

#include <memory>
#include <random>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits>
#include <vector>
#include <exception>
#include <string>

#include <boost/config/warning_disable.hpp>
#include <boost/move/move.hpp>
#include <boost/chrono.hpp>
#include <boost/noncopyable.hpp>
#include <boost/random.hpp>
#include <boost/preprocessor.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>

#include <boost/throw_exception.hpp>
#include <boost/exception/all.hpp>
#include <boost/utility/empty_deleter.hpp>

#include <boost/log/exceptions.hpp>
#include <boost/log/support/exception.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>


using namespace boost::assign;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
