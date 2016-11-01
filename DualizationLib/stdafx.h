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

#include <boost/config/warning_disable.hpp>
#include <boost/move/move.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/chrono.hpp>
#include <boost/noncopyable.hpp>
#include <boost/random.hpp>
#include <boost/preprocessor.hpp>
#include <boost/lexical_cast.hpp>

#include <boost\property_tree\ptree.hpp>
