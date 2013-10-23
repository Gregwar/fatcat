project(fatcat)

set(SOURCES
    main.cpp
    FatBackup.cpp
    FatChain.cpp
    FatChains.cpp
    FatEntry.cpp
    FatFilename.cpp
    FatPath.cpp
    FatSystem.cpp
    )

add_executable(fatcat ${SOURCES})
