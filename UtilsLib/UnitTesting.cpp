#include "stdafx.h"

#include "UnitTesting.h"
#include <CppUnitTest.h>


void UtilsLib::UT::Log(std::string const& message)
{
    Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(message.c_str());
}
