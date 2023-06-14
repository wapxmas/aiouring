cmake_minimum_required(VERSION 3.22)
project(aioutils)

set(CMAKE_CXX_STANDARD 20)

set(AIOUTILS_VERSION_MAJOR 1)
set(AIOUTILS_VERSION_MINOR 0)
set(AIOUTILS_VERSION_PATCH 0)

# Joins arguments and places the results in ${result_var}.
function(join result_var)
    set(result "")
    foreach (arg ${ARGN})
        set(result "${result}${arg}")
    endforeach ()
    set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

join(AIOUTILS_VERSION ${AIOUTILS_VERSION_MAJOR}.${AIOUTILS_VERSION_MINOR}.${AIOUTILS_VERSION_PATCH})

project(AIOUTILS VERSION ${AIOUTILS_VERSION}
        DESCRIPTION "AIOUring Utils"
        LANGUAGES CXX)

add_library(aioutils SHARED
        uenv.cpp
        ulinux.cpp
        utext.cpp
        uexcept.cpp
        unet.cpp
        include/aioutils/uhttp.hpp)

target_link_libraries(aioutils kklogging)

target_include_directories(aioutils PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(aioutils PROPERTIES
        VERSION ${AIOUTILS_VERSION}
        SOVERSION ${AIOUTILS_VERSION_MAJOR})
