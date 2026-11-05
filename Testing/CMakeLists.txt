cmake_minimum_required(VERSION 4.1)
project(TestingProject DESCRIPTION "Performance library testing.")

# Set CMake project options.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(PYTHON_TEST_MODE OFF CACHE BOOL "Use Python executable" FORCE)

# Add sources.
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/UnitTests.cpp)
add_executable(TestingProject ${SOURCES})

# Specify CMake subprojects.
add_subdirectory(${PROJECT_SOURCE_DIR}/Dependencies/GTest/googletest-1.17.0 EXCLUDE_FROM_ALL)
add_subdirectory(${PROJECT_SOURCE_DIR}/Dependencies/Visualise)

# Specify include paths.
target_include_directories(TestingProject PRIVATE
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/Dependencies/
        ${PROJECT_SOURCE_DIR}/Dependencies/Json/include
        ${PROJECT_SOURCE_DIR}/Dependencies/Json/include/nlohmann
        ${PROJECT_SOURCE_DIR}/Dependencies/GTest/googletest-1.17.0/googletest/include
        ${PROJECT_SOURCE_DIR}/Dependencies/Visualise/Visualise
)

# Define CMake variables.
target_compile_definitions(TestingProject PUBLIC
        SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        TEST_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/TestData/"
        TEST_DEPENDENCIES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/"
)

# Link CMake libraries.
target_link_libraries(TestingProject gtest gtest_main)
target_link_libraries(TestingProject LibraryProject)
target_link_libraries(TestingProject Visualise)

# Build info.
message(STATUS "Project Source Directory: ${PROJECT_SOURCE_DIR}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    # Compiler specific flags.
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS_RELEASE "/permissive- /GS /W3 /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt /Zc:forScope /openmp- /std:c++20 /FC /EHsc /nologo /diagnostics:column /O2 /Zi /MD  /D \"NDEBUG\" /MP /Oi /Ot")
        set(CMAKE_CXX_FLAGS_DEBUG   "/permissive- /GS /W3 /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt /Zc:forScope /openmp- /std:c++20 /FC /EHsc /nologo /diagnostics:column /Od /ZI /MDd /D \"_DEBUG\" /JMC /bigobj")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/ANIFEST /NXCOMPAT /DYNAMICBASE /DEBUG:FULL /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /ERRORREPORT:PROMPT /NOLOGO /MANIFESTUAC:NO /LTCG:incremental /OPT:ICF /OPT:REF")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG   "/MANIFEST /NXCOMPAT /DYNAMICBASE /DEBUG:FULL /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /ERRORREPORT:PROMPT /NOLOGO /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\" ")
    endif()
endif()