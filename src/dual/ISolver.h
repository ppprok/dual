#pragma once
#include "Configurable.h"

/**
    Interface of solvable task
*/
struct ITask {
    virtual ~ITask(){};
};

typedef std::shared_ptr<ITask> ITaskPtr;

/**
    Interface of common task solver
*/
struct ISolver {

    /// Solve task
    virtual void Solve(ITask* task) = 0;

    /// Get common description of result
    virtual std::string GetResultDescription() const = 0;

    virtual ~ISolver(){};
};

typedef std::shared_ptr<ISolver> ISolverPtr;