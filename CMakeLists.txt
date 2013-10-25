cmake_minimum_required(VERSION 2.8)
project(fatcat)

set(SOURCES
    main.cpp
    FatBackup.cpp
    FatChain.cpp
    FatChains.cpp
    FatDate.cpp
    FatDiff.cpp
    FatEntry.cpp
    FatFilename.cpp
    FatModule.cpp
    FatPath.cpp
    FatSystem.cpp
    )

foreach(SOURCE ${SOURCES})
    set(ALL_SOURCES ${ALL_SOURCES} src/${SOURCE})
endforeach()

add_executable(fatcat ${ALL_SOURCES})
install(TARGETS fatcat DESTINATION bin)
