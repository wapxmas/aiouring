cmake_minimum_required(VERSION 3.22)
project(aiouring)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/static)

set(CMAKE_CXX_STANDARD 20)

# Joins arguments and places the results in ${result_var}.
function(join result_var)
    set(result "")
    foreach (arg ${ARGN})
        set(result "${result}${arg}")
    endforeach ()
    set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

set(AIOURING_VERSION_MAJOR 1)
set(AIOURING_VERSION_MINOR 0)
set(AIOURING_VERSION_PATCH 0)

join(AIOURING_VERSION ${AIOURING_VERSION_MAJOR}.${AIOURING_VERSION_MINOR}.${AIOURING_VERSION_PATCH})

project(AIOUring VERSION ${AIOURING_VERSION}
        DESCRIPTION "Asynchronous IO_URING Framework"
        LANGUAGES CXX)

add_subdirectory(libs/fmt)
add_subdirectory(libs/kklogging)
add_subdirectory(libs/aioutils)

add_library(aiouring SHARED
        AIOUringTask.cpp
        AIOUring.cpp
        AIOUringOp.cpp
        include/aiouring/tasks/Http200ResponseTask.hpp
        include/aiouring/tasks/Http404ResponseTask.hpp
        include/aiouring/tasks/HttpJsonResponseTask.hpp
        include/aiouring/tasks/ResolveHostTask.hpp
        include/aiouring/tasks/TCPConnectTask.hpp
        include/aiouring/tasks/TCPInterweaveTask.hpp
        include/aiouring/tasks/TCPListeningTask.hpp
        include/aiouring/tasks/TCPSinkTask.hpp
        include/aiouring/tasks/TCPShutAndClose.hpp
        include/aiouring/tasks/TCPWrite.hpp
        )

target_link_libraries(aiouring aioutils kklogging fmt::fmt)

target_include_directories(aiouring PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(aiouring PROPERTIES
        VERSION ${AIOURING_VERSION}
        SOVERSION ${AIOURING_VERSION_MAJOR})
