set(SOURCE_FILES PosetsDualizationNode.h
        IntervalLattice.h
        PosetBase.h
        GeneticDualizer.cpp
        Callbacks.cpp
        MIGRAS.cpp
        WeightBoundsPruning.cpp
        Antichain.h
        FrequentInfrequentTask.cpp
        RUNC.h
        FrequentInfrequentEnumerator.cpp
        Callbacks.h
        IPoset.h
        RuncFixed.h
        RuncFixed.cpp
        SubsetsPoset.h
        PUNC.cpp
        NumberChain.cpp
        DualizationBacktrackAlgorithmBase.h
        Runc.cpp
        FileWriters.cpp
        FrequentInfrequentEnumerator.h
        Pruning.cpp
        Antichain.cpp
        PosetsProduct.cpp
        ReversedPoset.h
        Solver.cpp
        FrequentInfrequentTask.h
        ReversedPoset.cpp
        BoolMatrixPreparation.h
        AOT1.cpp
        PosetsDualizationNode.cpp
        PosetsProduct.h
        MostCoverCollector.cpp
        MostCoverCollector.h
        FileWriters.h
        PosetItem.cpp
        IPoset.cpp
        PosetBase.cpp
        GraphPoset.h
        WeightBoundsPruning.h
        WeightedCover.h
        Pruning.h
        SubsetsPoset.cpp
        AOT1.h
        IntervalLattice.cpp
        PosetsDualizationTask.h
        DualizationCounters.h
        MIGRAS.h
        DualizationCounters.cpp
        BoolMatrixPreparation.cpp
        CoverCollector.cpp
        GraphPoset.cpp
        NumberChain.h
        GeneticDualizer.h
        PUNC.h
        Weighters.h
        PosetItemsMatrix.h
        PosetItem.h
        CoverCollector.h
        PosetsDualizationTask.cpp
        ISolver.h
        Weighters.cpp
        PosetItemsMatrix.cpp
        DualizationBacktrackAlgorithmBase.cpp
        JYPDualizer.h
        Solver.h
        Counters.h)


add_library(dual ${SOURCE_FILES})

target_link_libraries(dual INTERFACE common)

add_executable(dualizer main.cpp)
target_link_libraries(dualizer dual common pthread)

add_library(runc SHARED RuncExecutor.cpp)
target_link_libraries(runc dual common)
set_property(TARGET runc PROPERTY POSITION_INDEPENDENT_CODE TRUE)