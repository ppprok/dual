#pragma once

#include "DualizationBacktrackAlgorithmBase.h"
#include "WeightedCover.h"

// Сбор перечисляемых покрытий
class CoverCollector : public IDualizationCallback {
protected:
    WeightedCovers _covers;

public:
    CoverCollector() : MaxCount(-1){};

    int MaxCount;

    virtual void Call(DualizationNode& node);

    WeightedCovers const& GetCovers() const;

    WeightedCovers MoveCovers();

    void Reset();

    void Reserve(int size);
};
