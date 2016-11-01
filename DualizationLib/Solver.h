#pragma once
#include "ISolver.h"
#include "MIGRAS.h"

namespace PosetDualization {namespace MIGRAS{

class Solver
    :public ISolver, public IConfigurable
{
    DualizationAlgorithm _algorithm;

   // PosetDualizationCallbacksPair<ItemsFilter, CSVFileWriter> _callbacksChain;

    ResultsCSVFileWriter _writer;

public: 

    virtual void Solve( ITask* task );

    static void Register();

    virtual void SetOptions(Options const& options);

    virtual void SetDefaultOptions();

    virtual std::string GetResultDescription() const;
};

}}