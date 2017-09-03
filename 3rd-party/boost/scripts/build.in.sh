#!/bin/bash -e

SOURCE_DIR="$1"
BINARY_DIR="$2"

cd "$SOURCE_DIR"/tools/build
./bootstrap.sh
./b2 install \
    --prefix="${CMAKE_CURRENT_BINARY_DIR}"

cd "$SOURCE_DIR"
"${CMAKE_CURRENT_BINARY_DIR}"/bin/b2 ${BOOST_MAKE_THREADS} \
    --with-system \
    --with-test \
    --with-log \
    --boost-build="${CMAKE_CURRENT_BINARY_DIR}"/share/boost-build/src/kernel \
    --build-dir="$BINARY_DIR" \
    --stagedir="${CMAKE_CURRENT_BINARY_DIR}" \
    link=static \
    cxxflags="${BOOST_CXX_FLAGS}" \
    ${BOOST_TARGET_OS} \
    ${BOOST_TOOLSET} \
    ${BOOST_ARCHITECTURE} \
    ${BOOST_ADDRESS_MODEL} \
    ${BOOST_LITTLE_ENDIAN} \
    stage
