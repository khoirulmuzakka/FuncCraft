cmake_minimum_required(VERSION 3.18)
project(FuncCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(FUNCCRAFT_BUILD_EXAMPLES "Build FuncCraft C++ examples" ON)
option(FUNCCRAFT_LINK_MINION "Build examples that use the Minion optimizer" ON)
option(FUNCCRAFT_BUILD_PYTHON "Build FuncCraft Python bindings" ON)

set(FUNCCRAFT_OUTPUT_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(FUNCCRAFT_OUTPUT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_library(funccraft STATIC
    src/basicf.cpp
    src/core.cpp
    src/support.cpp
    src/builder.cpp
    src/suite.cpp
    src/coordinate_transforms.cpp
    src/value_transforms.cpp
    src/composition.cpp
)

target_include_directories(funccraft PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(funccraft PUBLIC cxx_std_17)
set_target_properties(funccraft PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_BIN_DIR}"
)

if(WIN32)
    target_compile_definitions(funccraft PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(FUNCCRAFT_LINK_MINION)
    find_package(Minion QUIET CONFIG)
    if(NOT Minion_FOUND)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeLists.txt")
            FetchContent_Declare(
                minion
                SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.."
            )
        else()
            FetchContent_Declare(
                minion
                GIT_REPOSITORY https://github.com/khoirulmuzakka/Minion.git
                GIT_TAG main
                GIT_SHALLOW TRUE
            )
        endif()

        set(MINION_BUILD_BENCHMARK OFF CACHE BOOL "Build benchmark components" FORCE)
        set(MINION_BUILD_EXAMPLES OFF CACHE BOOL "Build example targets" FORCE)
        set(MINION_BUILD_TESTS OFF CACHE BOOL "Build test targets" FORCE)
        set(MINION_BUILD_PYTHON OFF CACHE BOOL "Disable Python extension" FORCE)
        FetchContent_MakeAvailable(minion)
    endif()

    if(TARGET minion)
        set_target_properties(minion PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_LIB_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_LIB_DIR}"
        )
    endif()
endif()

if(FUNCCRAFT_BUILD_EXAMPLES)
    function(funccraft_add_example target_name source_file)
        add_executable(${target_name} ${source_file})
        target_link_libraries(${target_name} PRIVATE funccraft ${ARGN})
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_BIN_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_BIN_DIR}"
        )
    endfunction()

    funccraft_add_example(check_basicf_optima examples/main_check_basicf_optima.cpp)
    if(FUNCCRAFT_LINK_MINION)
        funccraft_add_example(run_custom examples/main_run_custom.cpp minion)
    endif()
endif()

if(FUNCCRAFT_BUILD_PYTHON)
    find_package(pybind11 CONFIG QUIET)
    if(NOT pybind11_FOUND)
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.13.6
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(pybind11)
    endif()

    pybind11_add_module(_funccraft src/bindings.cpp)
    target_link_libraries(_funccraft PRIVATE funccraft)
    set_target_properties(_funccraft PROPERTIES
        OUTPUT_NAME "_funccraft"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/pyfunccraft"
    )
endif()
