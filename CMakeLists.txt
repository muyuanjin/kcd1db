cmake_minimum_required(VERSION 3.30)
project(kcd1db VERSION 0.1.0 LANGUAGES C CXX)

if (NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "kcd1db requires Windows x64 and MSVC")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "")
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "")
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "")
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "")
FetchContent_Declare(SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SQLiteCpp)

FetchContent_Declare(MinHook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG v1.3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(MinHook)

add_library(kcd1db SHARED src/Plugin.cpp src/LuaDB.cpp)
target_compile_options(kcd1db PRIVATE /utf-8 /W4)
target_compile_definitions(kcd1db PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX KCD1DB_VERSION="${PROJECT_VERSION}")
target_link_libraries(kcd1db PRIVATE SQLiteCpp minhook ole32)
set_target_properties(kcd1db PROPERTIES SUFFIX ".asi")
