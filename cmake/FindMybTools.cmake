# Discovery is intentionally local and offline. Installations are supplied through
# PATH or the normal CMake cache arguments; no machine paths enter this module.
if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "MYB currently supports the native Ninja generator")
endif()
execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" --version
    OUTPUT_VARIABLE MYB_NINJA_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE MYB_NINJA_RESULT)
if(NOT MYB_NINJA_RESULT EQUAL 0 OR MYB_NINJA_VERSION VERSION_LESS 1.11)
    message(FATAL_ERROR "MYB requires Ninja 1.11 or newer")
endif()
find_package(BISON 3.8.2 REQUIRED)
find_package(FLEX 2.6.4 REQUIRED)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
message(STATUS "MYB tools: Ninja ${MYB_NINJA_VERSION}; Python ${Python3_VERSION}; Flex ${FLEX_VERSION}; Bison ${BISON_VERSION}")
