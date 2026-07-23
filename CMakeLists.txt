cmake_minimum_required(VERSION 3.18)
project(FuncCraft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)
include(GNUInstallDirs)

option(BUILD_LIBRARY "Build the FuncCraft C++ library" ON)
option(BUILD_EXAMPLES "Build FuncCraft C++ examples, including Minion-dependent examples" OFF)
option(BUILD_PYTHON "Build the Python funccraft interface" ON)
option(BUILD_TEST "Build FuncCraft C++ test targets" OFF)
option(FUNCCRAFT_INSTALL "Install FuncCraft targets and package metadata" OFF)

if((BUILD_EXAMPLES OR BUILD_PYTHON OR BUILD_TEST OR FUNCCRAFT_INSTALL) AND NOT BUILD_LIBRARY)
    message(FATAL_ERROR "BUILD_LIBRARY must be ON when BUILD_EXAMPLES, BUILD_PYTHON, BUILD_TEST, or FUNCCRAFT_INSTALL is ON.")
endif()

if(PROJECT_IS_TOP_LEVEL AND NOT (DEFINED SKBUILD AND SKBUILD))
    set(FUNCCRAFT_OUTPUT_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
    set(FUNCCRAFT_OUTPUT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
    set(FUNCCRAFT_PYTHON_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/funccraft")
else()
    set(FUNCCRAFT_OUTPUT_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin")
    set(FUNCCRAFT_OUTPUT_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib")
    set(FUNCCRAFT_PYTHON_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/funccraft")
endif()

find_package(yaml-cpp CONFIG QUIET)
set(_funccraft_fetched_yaml_cpp OFF)
if(TARGET yaml-cpp::yaml-cpp)
    set(FUNCCRAFT_YAML_CPP_TARGET yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    set(FUNCCRAFT_YAML_CPP_TARGET yaml-cpp)
else()
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
        GIT_SHALLOW TRUE
    )
    set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "Build yaml-cpp as a shared library" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "Build yaml-cpp contrib" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Build yaml-cpp tools" FORCE)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Build yaml-cpp tests" FORCE)
    FetchContent_MakeAvailable(yaml-cpp)
    set(_funccraft_fetched_yaml_cpp ON)
    set(FUNCCRAFT_YAML_CPP_TARGET yaml-cpp)
endif()

file(GLOB FUNCCRAFT_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM FUNCCRAFT_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp")

if(BUILD_LIBRARY)
    add_library(funccraft STATIC ${FUNCCRAFT_SOURCE_FILES})

    target_include_directories(funccraft PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
    )

    target_compile_features(funccraft PUBLIC cxx_std_17)
    target_link_libraries(funccraft PUBLIC
        $<BUILD_INTERFACE:${FUNCCRAFT_YAML_CPP_TARGET}>
        $<INSTALL_INTERFACE:yaml-cpp::yaml-cpp>
    )
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
    target_compile_definitions(funccraft PRIVATE
        FUNCCRAFT_DEFAULT_SUITE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/suites"
        FUNCCRAFT_INSTALLED_SUITE_DIR="${CMAKE_INSTALL_FULL_DATADIR}/FuncCraft/suites"
    )
endif()

if(BUILD_EXAMPLES)
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

if(BUILD_EXAMPLES)
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

    funccraft_add_example(check_optima examples/main_check_optima.cpp)
    funccraft_add_example(run_minimize examples/main_minimize.cpp minion)
    funccraft_add_example(run_experiments examples/main_run_experiments.cpp minion)
endif()

if(BUILD_TEST)
    enable_testing()
    add_executable(funccraft_test tests/main_test.cpp)
    target_link_libraries(funccraft_test PRIVATE funccraft)
    target_compile_definitions(funccraft_test PRIVATE FUNCCRAFT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(funccraft_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${FUNCCRAFT_OUTPUT_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_OUTPUT_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_OUTPUT_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_OUTPUT_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_OUTPUT_BIN_DIR}"
    )
    add_test(NAME funccraft_test COMMAND funccraft_test)

endif()

if(BUILD_PYTHON)
    find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m pybind11 --cmakedir
        OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
        RESULT_VARIABLE PYBIND11_CMAKE_DIR_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(PYBIND11_CMAKE_DIR_RESULT EQUAL 0 AND EXISTS "${PYBIND11_CMAKE_DIR}")
        set(pybind11_DIR "${PYBIND11_CMAKE_DIR}")
    endif()

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

    file(MAKE_DIRECTORY "${FUNCCRAFT_PYTHON_OUTPUT_DIR}")
    pybind11_add_module(_funccraft src/bindings.cpp)
    target_link_libraries(_funccraft PRIVATE funccraft)
    set_target_properties(_funccraft PROPERTIES
        OUTPUT_NAME "_funccraft"
        LIBRARY_OUTPUT_DIRECTORY "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FUNCCRAFT_PYTHON_OUTPUT_DIR}"
    )
endif()

if(DEFINED SKBUILD AND SKBUILD)
    if(TARGET _funccraft)
        install(TARGETS _funccraft
            LIBRARY DESTINATION funccraft
            RUNTIME DESTINATION funccraft
        )
    endif()
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/suites/
        DESTINATION funccraft/suites
        FILES_MATCHING PATTERN "*.yaml" PATTERN "*.yml"
    )
elseif(FUNCCRAFT_INSTALL)
    if(_funccraft_fetched_yaml_cpp)
        message(FATAL_ERROR "FUNCCRAFT_INSTALL requires yaml-cpp from find_package(yaml-cpp CONFIG). Disable FUNCCRAFT_INSTALL or install yaml-cpp first.")
    endif()

    include(CMakePackageConfigHelpers)

    install(TARGETS funccraft
        EXPORT FuncCraftTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/suites/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/FuncCraft/suites
        FILES_MATCHING PATTERN "*.yaml" PATTERN "*.yml"
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/FuncCraftConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FuncCraftConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/FuncCraftConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FuncCraft
    )

    install(EXPORT FuncCraftTargets
        FILE FuncCraftTargets.cmake
        NAMESPACE FuncCraft::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FuncCraft
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/FuncCraftConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/FuncCraftConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FuncCraft
    )
endif()
