add_executable(${MAIN_EXECUTABLE_NAME}
	main.cpp
)

target_link_libraries(${MAIN_EXECUTABLE_NAME}
	PUBLIC Boost::program_options
	PUBLIC Boost::headers
)

target_include_directories(${MAIN_EXECUTABLE_NAME}
	PUBLIC "${CMAKE_SOURCE_DIR}/include"
)

add_subdirectory(parser)
add_subdirectory(terms)

target_link_libraries(${MAIN_EXECUTABLE_NAME}
	PRIVATE ${MAIN_EXECUTABLE_NAME}::parser
	PRIVATE ${MAIN_EXECUTABLE_NAME}::terms
)
