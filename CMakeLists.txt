cmake_minimum_required(VERSION 3.30)
project(pg_email_opt C)

set(CMAKE_C_STANDARD 11)

set(PostgreSQL_INCLUDE_DIR "/opt/homebrew/opt/postgresql@16/include")
set(PostgreSQL_LIBRARY "/opt/homebrew/opt/postgresql@16/lib/libpq.dylib")

find_package(PostgreSQL REQUIRED)

if(PostgreSQL_FOUND)
    message(STATUS "Found PostgreSQL version: ${PostgreSQL_VERSION_STRING}")
    message(STATUS "PostgreSQL include directories: ${PostgreSQL_INCLUDE_DIRS}")
    message(STATUS "PostgreSQL libraries: ${PostgreSQL_LIBRARIES}")
endif()

add_library(pg_email_opt STATIC pg_email_opt.c
        myutils/local.h
        myutils/domain.h
        myutils/ip.h
        myutils/common.h)

target_include_directories(pg_email_opt PRIVATE
        ${PostgreSQL_INCLUDE_DIR}
        ${PostgreSQL_INCLUDE_DIR}/postgresql
        ${PostgreSQL_INCLUDE_DIR}/postgresql/server
)

target_link_libraries(pg_email_opt PRIVATE
        ${PostgreSQL_LIBRARIES}
)
