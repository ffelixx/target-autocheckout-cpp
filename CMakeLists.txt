cmake_minimum_required(VERSION 3.15)
project(TargetBot VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/models/Task.cpp
    src/network/HttpClient.cpp
    src/network/WebhookClient.cpp
    src/utils/FileReader.cpp
    src/utils/ProxyParser.cpp
    src/utils/ConfigLoader.cpp
    src/utils/Logger.cpp
)

# Create executable
add_executable(TargetBot ${SOURCES})

# Link libraries
target_link_libraries(TargetBot
    PRIVATE
        ${CURL_LIBRARIES}
        nlohmann_json::nlohmann_json
)

# Platform specific settings
if(WIN32)
    target_link_libraries(TargetBot PRIVATE ws2_32)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(TargetBot PRIVATE /W4)
else()
    target_compile_options(TargetBot PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation
install(TARGETS TargetBot
    RUNTIME DESTINATION bin
)
