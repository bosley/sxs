add_library(map SHARED map.c)

install(TARGETS map
    LIBRARY DESTINATION libs/map/lib
    ARCHIVE DESTINATION libs/map/lib
)

install(FILES map.h
    DESTINATION libs/map/include
)

add_subdirectory(tests)

