include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        23ef29555ef4789f555f1ba8c51b4c52975f0907
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set(TEST_FILE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/input_files")

add_compile_definitions(TEST_FILE_DIRECTORY=${TEST_FILE_DIRECTORY})

# Add the current dir to the include directories for all test cases
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(utils)
add_subdirectory(parser)
add_subdirectory(terms)
add_subdirectory(processor)
