set(SOURCE_FILES io_utils.cpp
        IntegersFileWriter.h
        io_utils.h
        chunk.h
        string_utils.h
        marked.h
        raw_vector.h
        graph_algorithms.cpp
        graph_algorithms.h
        IteratorRange.h
        bit_vector.h
        logging_utils.cpp
        ReadMe.txt
        bit_chunk.h
        bits.h
        IntegersFileReader.cpp
        blob.h
        IntegersFileReader.h
        containers.h
        IntegersFileWriter.cpp
        errors.h
        range.h
        Configurable.h
        CSVLoader.cpp
        logging_utils.h
        Configurable.cpp
        CSVLoader.h
        bit_mask.h
        math_utils.h
        Registrator.h
        bit_matrix.h
        Randomizer.h
        bits.cpp)

add_library(common ${SOURCE_FILES})
target_link_libraries(common PUBLIC boost)

