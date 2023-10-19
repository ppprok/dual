add_executable(TestDual
#        MIGRASIntervalsTests.cpp
#        RuncFixedTests.cpp
#        PosetSelfTester.cpp
#        AOT1Tests.cpp
#        MIGRASTests.cpp
#        GeneticDualizerTests.cpp
#        DualPosetsTaskTests.cpp
#        FileWritersTests.cpp
#        GraphPosetTests.cpp
        RUNCTests.cpp
        main.cpp
        #        CoverCollectorsTests.cpp
#        IntervalLatticeTests.cpp
#        PruningTests.cpp
#        FrequentInfrequentTests.cpp
#        PosetSelfTester.h
#        NumberChainPosetTests.cpp
#        PUNCTests.cpp
#        AntichainTests.cpp
)

target_link_libraries(TestDual PRIVATE dual)

add_dependencies(TestDual doctest)
target_include_directories(TestDual PRIVATE ${DOCTEST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)

add_dependencies(TestDual InstallTestUtilsData)