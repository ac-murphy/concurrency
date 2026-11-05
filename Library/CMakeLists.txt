cmake_minimum_required(VERSION 4.1)

project(LibraryProject DESCRIPTION "Portfolio library.")

set(CMAKE_VERBOSE_MAKEFILE ON)
option(TRACY_PROFILER_GLOBAL "TRACY PROFILER" OFF)
message(STATUS "TRACY PROFILER: ${TRACY_PROFILER_GLOBAL}")

message(STATUS "Project Source Directory: ${PROJECT_SOURCE_DIR}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tracy Profiler: ${TRACY_PROFILER_CORE}")

set(CMAKE_CXX_FLAGS_RELEASE "/permissive- /GS /W3 /WX /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt /Zc:forScope /std:c++20 /FC /EHsc /nologo /diagnostics:column /O2 /Zi /MD  /D \"NDEBUG\" /MP /Oi /Ot")
set(CMAKE_CXX_FLAGS_DEBUG   "/permissive- /GS /W3 /WX /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt /Zc:forScope /std:c++20 /FC /EHsc /nologo /diagnostics:column /Od /ZI /MDd /D \"_DEBUG\" /JMC")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/MANIFEST /NXCOMPAT /DYNAMICBASE /DEBUG:FULL /DLL /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /ERRORREPORT:PROMPT /NOLOGO /MANIFESTUAC:NO /LTCG:incremental /OPT:ICF /OPT:REF")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG   "/MANIFEST /NXCOMPAT /DYNAMICBASE /DEBUG:FULL /DLL /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /ERRORREPORT:PROMPT /NOLOGO /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\" ")

file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "${PROJECT_SOURCE_DIR}/Dependencies")
if ("${TRACY_PROFILER_GLOBAL}" STREQUAL "ON")
    list(APPEND SOURCES ${PROJECT_SOURCE_DIR}/Dependencies/Tracy/Tracy_0.9.1/public/TracyClient.cpp)
endif()

add_library(LibraryProject STATIC ${SOURCES})

target_compile_definitions(LibraryProject PUBLIC
        OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Output/"
)
if ("${TRACY_PROFILER_GLOBAL}" STREQUAL "ON")
    target_compile_definitions(LibraryProject PRIVATE TRACY_ENABLE=1 TRACY_NO_EXIT=1)
    add_subdirectory(${PROJECT_SOURCE_DIR}/Dependencies/Tracy/Tracy_0.9.1)
endif ()

target_include_directories(LibraryProject PUBLIC
        ${PROJECT_SOURCE_DIR}/
        ${PROJECT_SOURCE_DIR}/Dependencies/Json/include
        ${PROJECT_SOURCE_DIR}/Dependencies/Eigen/eigen-5.0.0/Eigen
        if ("${TRACY_PROFILER_GLOBAL}" STREQUAL "ON")
        ${PROJECT_SOURCE_DIR}/Dependencies/Tracy/Tracy_0.9.1/public/tracy/
        endif ()
)


if ("${TRACY_PROFILER_GLOBAL}" STREQUAL "ON")
    target_link_libraries(LibraryProject PUBLIC
            Tracy::TracyClient
    )
else()
    target_link_libraries(LibraryProject)
endif()