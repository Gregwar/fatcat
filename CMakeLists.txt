project(fatscan)

set(SOURCES
    main.cpp
    FatSystem.cpp
    FatEntry.cpp
    FatFilename.cpp
    )

add_executable(fatscan ${SOURCES})
