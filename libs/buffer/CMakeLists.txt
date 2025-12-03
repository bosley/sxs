add_library(buffer SHARED buffer.c)

install(TARGETS buffer
    LIBRARY DESTINATION libs/buffer/lib
    ARCHIVE DESTINATION libs/buffer/lib
)

install(FILES buffer.h
    DESTINATION libs/buffer/include
)

add_subdirectory(tests)

