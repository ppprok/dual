#include "DualizationCounters.h"

#include <sstream>

std::string DualizationCounters::ToString() const {
    std::stringstream ss;
    ss << "inner:" << inners << "extra: " << extras << "count " << results << "max delay: " << maxDelay;
    return ss.str();
}

DualizationCounters::DualizationCounters() {
    Reset();
}

void DualizationCounters::Reset() {
    results = 0;
    inners = 0;
    extras = 0;
    maxDelay = 0;
    currentDelay = 0;
}