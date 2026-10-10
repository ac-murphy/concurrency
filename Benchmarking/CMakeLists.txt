cmake_minimum_required(VERSION 4.1)
project(BenchmarkingProject DESCRIPTION "Performance library benchmarking")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS_RELEASE "/permissive- /GS /W3 /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt /Zc:forScope /openmp- /std:c++20 /FC /EHsc /nologo /diagnostics:column /O2 /Zi /MD  /D \"NDEBUG\" /MP /Oi /Ot")
set(CMAKE_CXX_FLAGS_DEBUG   "/permissive- /GS /W3 /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt /Zc:forScope /openmp- /std:c++20 /FC /EHsc /nologo /diagnostics:column /Od /ZI /MDd /D \"_DEBUG\" /JMC /bigobj")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/ANIFEST /NXCOMPAT /DYNAMICBASE /DEBUG:FULL /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /ERRORREPORT:PROMPT /NOLOGO /MANIFESTUAC:NO /LTCG:incremental /OPT:ICF /OPT:REF")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG   "/MANIFEST /NXCOMPAT /DYNAMICBASE /DEBUG:FULL /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /ERRORREPORT:PROMPT /NOLOGO /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\" ")

message(STATUS "Project Source Directory: ${PROJECT_SOURCE_DIR}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/Dependencies/benchmark-main EXCLUDE_FROM_ALL)

add_executable(BenchmarkingProject
        GreeksBenchmark.cpp)

target_include_directories(BenchmarkingProject PRIVATE
        ${PROJECT_SOURCE_DIR}/Dependencies/benchmark-main/include/
)

target_compile_definitions(BenchmarkingProject PUBLIC
        SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        TEST_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/TestData/"
        TEST_DEPENDENCIES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/"
)

target_link_libraries(BenchmarkingProject benchmark::benchmark)
target_link_libraries(BenchmarkingProject LibraryProject)

message("debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message("release flags: ${CMAKE_CXX_FLAGS_RELEASE}")