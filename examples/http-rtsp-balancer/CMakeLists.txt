cmake_minimum_required(VERSION 3.22)

project(VSBalancer VERSION 1.0
        DESCRIPTION "Video Services Balancer")

configure_file(VSBalancer.h.in VSBalancer.h)

set(EXECUTABLE_NAME vs-balancer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unknown-pragmas -Wno-switch -Wno-unused-function -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-unused-label")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

add_link_options("-fuse-ld=mold")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/static)

add_custom_target(copy-config ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/etc
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/etc
        )

add_subdirectory(./libs/fmt ${PROJECT_BINARY_DIR}/fmt)
add_subdirectory(./libs/aioutils ${PROJECT_BINARY_DIR}/aioutils)
add_subdirectory(./libs/aiouring ${PROJECT_BINARY_DIR}/aiouring)
add_subdirectory(./libs/kklogging ${PROJECT_BINARY_DIR}/kklogging)
add_subdirectory(./libs/libpqxx-7.7.3 ${PROJECT_BINARY_DIR}/libpqxx)

add_executable(${EXECUTABLE_NAME} VSBalancer.cpp)

target_link_libraries(${EXECUTABLE_NAME} fmt::fmt aioutils aiouring kklogging uring mimalloc pqxx pq)

add_dependencies(${EXECUTABLE_NAME} copy-config)

target_include_directories(${EXECUTABLE_NAME}
        PRIVATE ${PROJECT_BINARY_DIR}
        PUBLIC ${PROJECT_SOURCE_DIR}/include)
