cmake_minimum_required(VERSION 3.30)
project(pg_email_opt C)
set(CMAKE_C_STANDARD 11)

# Enable position independent code for shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find PostgreSQL package
find_package(PostgreSQL 16 REQUIRED)

# Get PostgreSQL specific directories using pg_config
execute_process(
        COMMAND pg_config --includedir-server
        OUTPUT_VARIABLE PG_SERVER_INCLUDEDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND pg_config --pkglibdir
        OUTPUT_VARIABLE PG_PKGLIBDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND pg_config --sharedir
        OUTPUT_VARIABLE PG_SHAREDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Print found paths for verification
message(STATUS "PostgreSQL version: ${PostgreSQL_VERSION_STRING}")
message(STATUS "PostgreSQL server include dir: ${PG_SERVER_INCLUDEDIR}")
message(STATUS "PostgreSQL package lib dir: ${PG_PKGLIBDIR}")
message(STATUS "PostgreSQL share dir: ${PG_SHAREDIR}")

# Source files list
set(SOURCE_FILES
        pg_email_opt.c
        myutils/ip.c
        myutils/domain.c
        myutils/common.c
        myutils/local.c
)

# Header files list
set(HEADER_FILES
        myutils/local.h
        myutils/domain.h
        myutils/ip.h
        myutils/common.h
)

# Create shared library (MODULE for PostgreSQL extension)
add_library(pg_email_opt MODULE ${SOURCE_FILES} ${HEADER_FILES})

# Set include directories
target_include_directories(pg_email_opt PRIVATE
        ${PostgreSQL_INCLUDE_DIRS}
        ${PG_SERVER_INCLUDEDIR}
        ${CMAKE_CURRENT_SOURCE_DIR}  # Add current directory for myutils includes
)

# Link PostgreSQL libraries
target_link_libraries(pg_email_opt PRIVATE
        ${PostgreSQL_LIBRARIES}
)

# Add PostgreSQL specific compile definitions
target_compile_definitions(pg_email_opt PRIVATE
        _GNU_SOURCE
)

# Set output directory
set_target_properties(pg_email_opt PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Set output properties for the library
set_target_properties(pg_email_opt PROPERTIES
        PREFIX ""  # Remove 'lib' prefix
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
