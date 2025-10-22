cmake_minimum_required(VERSION 3.16)
project(CppWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- CPM.cmake Dependency Manager ---
# Downloads the script if it's not found.
set(CPM_DOWNLOAD_VERSION 0.42.0)
if(NOT (CPM_SOURCE_CACHE OR DEFINED ENV{CPM_SOURCE_CACHE}))
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}...")
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
    )
endif()
include(${CPM_DOWNLOAD_LOCATION})

# --- Find Dependencies ---
set(BOOST_DOWNLOAD_VERSION 1.89.0)
CPMAddPackage(
    NAME Boost
    VERSION ${BOOST_DOWNLOAD_VERSION}
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-${BOOST_DOWNLOAD_VERSION}
    GIT_SHALLOW TRUE
)

set(WXWIDGETS_DOWNLOAD_VERSION 3.3.1)
CPMAddPackage(
    NAME wxWidgets
    GITHUB_REPOSITORY wxWidgets/wxWidgets
    GIT_TAG v${WXWIDGETS_DOWNLOAD_VERSION}
    OPTIONS
        "wxBUILD_SAMPLES OFF"
        "wxBUILD_DEMOS OFF"
        "wxBUILD_TESTS OFF"
        "wxUSE_GUI ON"
)

set(GSL_DOWNLOAD_VERSION 4.2.0)
CPMAddPackage(
    NAME GSL
    GITHUB_REPOSITORY "Microsoft/GSL"
    GIT_TAG v${GSL_DOWNLOAD_VERSION} # A recent stable version
    OPTIONS "GSL_TEST=OFF" # Don't build tests
)

# --- Platform Specifics for Future ATEM SDK ---
# This is a placeholder for your future integration.
if(WIN32)
    message(STATUS "Configuring for Windows")
    # Add COM libraries if needed, e.g., target_link_libraries(${PROJECT_NAME} PRIVATE ole32 oleaut32)
elseif(APPLE)
    message(STATUS "Configuring for macOS")
    # Add CoreFoundation framework
    find_library(CORE_FOUNDATION_LIB CoreFoundation)
    if(NOT CORE_FOUNDATION_LIB)
        message(FATAL_ERROR "CoreFoundation not found on macOS")
    endif()
endif()

# --- Source Files ---
file(GLOB_RECURSE APP_CPP_SOURCES "src/*.cpp")
file(GLOB_RECURSE APP_HPP_SOURCES "src/*.hpp")
set(APP_SOURCES ${APP_CPP_SOURCES} ${APP_HPP_SOURCES})


# --- Executable Target ---
add_executable(${PROJECT_NAME} ${APP_SOURCES})

# On Windows, create a windowed application instead of a console one
if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE ON)
endif()

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::system # Required by Asio
    Boost::filesystem
    wx::core
    wx::base
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CORE_FOUNDATION_LIB})
endif()

# C++20 requires this for Boost.Asio on some compilers
if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    ${Boost_INCLUDE_DIRS}
    ${wxWidgets_INCLUDE_DIRS}
    "${GSL_SOURCE_DIR}/include" # Correct way to get GSL includes with CPM
)

# --- Static Analysis and Formatting ---
# Generate compile_commands.json for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    add_custom_target(
        format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${APP_SOURCES}
        COMMENT "Formatting code with clang-format..."
        VERBATIM
    )
else()
    message(STATUS "clang-format not found. The 'format' target will not be available.")
endif()

find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    add_custom_target(
        tidy
        COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} ${APP_CPP_SOURCES}
        COMMENT "Running clang-tidy..."
        VERBATIM
    )
else()
    message(STATUS "clang-tidy not found. The 'tidy' target will not be available.")
endif()
