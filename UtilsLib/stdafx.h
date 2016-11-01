// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _SECURE_SCL 0
#define _SCL_SECURE_NO_WARNINGS
#define _CRT_DISABLE_PERFCRIT_LOCKS

#include "targetver.h"

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

#include <filesystem>

#include <boost/config/warning_disable.hpp>
#include <boost/move/move.hpp>
#include <boost/chrono.hpp>
#include <boost/noncopyable.hpp>
#include <boost/random.hpp>
#include <boost/preprocessor.hpp>

#include <boost/throw_exception.hpp>
#include <boost/exception/all.hpp>

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

#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\info_parser.hpp>
#include <boost\property_tree\json_parser.hpp>
#include <boost\property_tree\xml_parser.hpp>
#include <boost\property_tree\ini_parser.hpp>
