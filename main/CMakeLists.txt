set(INCLUDEDIRS
    ./
)

set(SRCS
    main.cpp
    module_rca_i2s.c
    chipstream.c
)

idf_component_register(
    INCLUDE_DIRS ${INCLUDEDIRS}
    SRCS ${SRCS}
    REQUIRES arduino m5stack m5gfx ymfm chipstream)
