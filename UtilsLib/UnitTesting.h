#pragma once

#include <string>
#include <boost\format.hpp>

namespace UtilsLib
{ 
    namespace UT
    {

        void Log(std::string const& message);

        template <typename T1>
        void Log(std::string const& format, T1 const& v1)
        {
            Log(boost::str(boost::format(format)%v1));
        }

        template <typename T1, typename T2>
        void Log(std::string const& format, T1 const& v1, T2 const& v2)
        {
            Log(boost::str(boost::format(format)%v1%v2));
        }
                

    }
}