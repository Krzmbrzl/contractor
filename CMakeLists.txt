cmake_minimum_required(VERSION 3.15)

project(masters_thesis_program,
	VERSION "0.1.0"
	LANGUAGES "CXX"
)

# Use cpp17 and error if that is not available
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Make sure cmake finds the extra scripts in the cmake subdirectory
list(APPEND CMAKE_MODULE_PATH
	"${CMAKE_SOURCE_DIR}/cmake"
)

# Make sure that libraries and executables end up in the correct directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Create a compile_commands.json DB for use with clang-tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup and configure all dependencies
include(setup_dependencies)


set(MAIN_EXECUTABLE_NAME "contractor")


add_subdirectory(src)
