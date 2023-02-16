cmake_minimum_required(VERSION 3.5)

set(INCLUDEDIRS
    ymfm/src/
)

set(SRCS
    ymfm/src/ymfm_adpcm.cpp
    ymfm/src/ymfm_misc.cpp
    ymfm/src/ymfm_opl.cpp
    ymfm/src/ymfm_opm.cpp
    ymfm/src/ymfm_opn.cpp
    ymfm/src/ymfm_opq.cpp
    ymfm/src/ymfm_opz.cpp
    ymfm/src/ymfm_pcm.cpp
    ymfm/src/ymfm_ssg.cpp
    ffi/ymfmffi.cpp
)

idf_component_register(
    INCLUDE_DIRS ${INCLUDEDIRS}
    SRCS ${SRCS}
)

target_compile_options(${COMPONENT_TARGET} PRIVATE
    -O3
    -std=c++14
    -fno-exceptions
    -Wno-array-bounds
    -fPIC
)
