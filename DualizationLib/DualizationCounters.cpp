#include "stdafx.h"
#include "DualizationCounters.h"


std::string DualizationCounters::ToString() const
{
    return boost::str(boost::format("inner: %1%\nextra: %2%\ncount: %3%\nmax delay: %4%\n")%inners %extras% results%maxDelay);
}

DualizationCounters::DualizationCounters()
{
    Reset();
}

void DualizationCounters::Reset()
{
    results = 0;
    inners = 0;
    extras = 0;
    maxDelay = 0;
    currentDelay = 0;
}