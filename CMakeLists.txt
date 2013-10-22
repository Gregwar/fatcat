project(fatscan)

set(SOURCES
    main.cpp
    FatEntry.cpp
    FatFilename.cpp
    FatPath.cpp
    FatSystem.cpp
    )

add_executable(fatscan ${SOURCES})
