set(LIB_ALIAS "utils")
set(LIB_NAME "${MAIN_EXECUTABLE_NAME}_${LIB_ALIAS}")

add_library(${LIB_NAME} STATIC
	IndexSpaceResolver.cpp
	PairingGenerator.cpp
	TermList.cpp
)

add_library(${MAIN_EXECUTABLE_NAME}::${LIB_ALIAS} ALIAS ${LIB_NAME})

target_include_directories(${LIB_NAME}
	PUBLIC "${CMAKE_SOURCE_DIR}/include"
)
target_link_libraries(${LIB_NAME}
	PUBLIC ${MAIN_EXECUTABLE_NAME}::terms
)
