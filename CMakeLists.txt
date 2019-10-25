cmake_minimum_required(VERSION 2.8)
project(fatcat)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -s")
endif()

set(SOURCES
    core/FatEntry.cpp
    core/FatFilename.cpp
    core/FatModule.cpp
    core/FatPath.cpp
    core/FatSystem.cpp
    core/FatDate.cpp

    table/FatBackup.cpp
    table/FatDiff.cpp

    analysis/FatExtract.cpp
    analysis/FatFix.cpp
    analysis/FatChain.cpp
    analysis/FatChains.cpp
    analysis/FatSearch.cpp
    analysis/FatWalk.cpp

    xgetopt/xgetopt.cpp
    xgetopt/stdafx.cpp
    xgetopt/argcargv.cpp

    mingw/utimes.cpp
    )

foreach(SOURCE ${SOURCES})
    set(ALL_SOURCES ${ALL_SOURCES} src/${SOURCE})
endforeach()

include_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(fatcat "src/fatcat.cpp" ${ALL_SOURCES})
install(TARGETS fatcat DESTINATION bin)
