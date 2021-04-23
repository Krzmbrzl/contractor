include(GoogleTest)

add_executable(parser_test
	ParserTest.cpp
)

target_link_libraries(parser_test
	gtest_main
	${MAIN_EXECUTABLE_NAME}::parser
)

gtest_discover_tests(parser_test)
