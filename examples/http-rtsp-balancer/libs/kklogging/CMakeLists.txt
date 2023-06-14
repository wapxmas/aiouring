cmake_minimum_required(VERSION 3.22)
project(kklogging)

set(CMAKE_CXX_STANDARD 20)

set(KKLOGGING_VERSION_MAJOR 1)
set(KKLOGGING_VERSION_MINOR 0)
set(KKLOGGING_VERSION_PATCH 0)

# Joins arguments and places the results in ${result_var}.
function(join result_var)
    set(result "")
    foreach (arg ${ARGN})
        set(result "${result}${arg}")
    endforeach ()
    set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

join(KKLOGGING_VERSION ${KKLOGGING_VERSION_MAJOR}.${KKLOGGING_VERSION_MINOR}.${KKLOGGING_VERSION_PATCH})

project(KKLogging VERSION ${KKLOGGING_VERSION}
        DESCRIPTION "Kevin Kreiser Logging"
        LANGUAGES CXX)

add_library(kklogging SHARED kklogging.cpp)

target_link_libraries(kklogging fmt::fmt)

target_include_directories(kklogging PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(kklogging PROPERTIES
        VERSION ${KKLOGGING_VERSION}
        SOVERSION ${KKLOGGING_VERSION_MAJOR})
