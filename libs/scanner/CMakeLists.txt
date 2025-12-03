add_library(scanner SHARED scanner.c)

target_link_libraries(scanner PUBLIC buffer)

install(TARGETS scanner
    LIBRARY DESTINATION libs/scanner/lib
    ARCHIVE DESTINATION libs/scanner/lib
)

install(FILES scanner.h
    DESTINATION libs/scanner/include
)

add_subdirectory(tests)

