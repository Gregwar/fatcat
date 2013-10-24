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

add_executable(fatcat ${SOURCES})

install(TARGETS fatcat DESTINATION bin)
