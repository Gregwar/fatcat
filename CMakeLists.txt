cmake_minimum_required(VERSION 3.5)
project(fatcat)

OPTION(DEFINE_WIN "Compiling windows" OFF)

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
    )

foreach(SOURCE ${SOURCES})
    set(ALL_SOURCES ${ALL_SOURCES} src/${SOURCE})
endforeach()

IF(DEFINE_WIN)
	add_definitions(-D__WIN__)
ENDIF(DEFINE_WIN)

include_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(fatcat "src/fatcat.cpp" ${ALL_SOURCES})
install(TARGETS fatcat DESTINATION bin)
