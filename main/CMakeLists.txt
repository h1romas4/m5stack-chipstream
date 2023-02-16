set(INCLUDEDIRS
    ./
)

set(SRCS
    main.cpp
)

idf_component_register(
    INCLUDE_DIRS ${INCLUDEDIRS}
    SRCS ${SRCS}
    REQUIRES arduino m5stack m5gfx)
