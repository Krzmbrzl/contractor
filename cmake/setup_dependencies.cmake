include(FetchContent)

# Boost
# We only have to specify the libs that are NOT header-only as dependencies
find_package(Boost REQUIRED COMPONENTS program_options)

if (${Boost_VERSION_STRING} VERSION_LESS "1.67.0")
	message(STATUS "Detected Boost version is not recent enough to support the is_detected type-trait. Including packaged headers into search path...")

	FetchContent_Declare(
		boost_type_traits
		GIT_REPOSITORY https://github.com/boostorg/type_traits.git
		GIT_TAG boost-1.67.0
	)

	FetchContent_GetProperties(boost_type_traits)
	if(NOT boost_type_traits_POPULATED)
		FetchContent_Populate(boost_type_traits)
		include_directories(AFTER "${boost_type_traits_SOURCE_DIR}/include/")
	endif()
endif()

# nlohmann-JSON
FetchContent_Declare(
	json
	GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
	GIT_TAG v3.9.1
)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
	FetchContent_Populate(json)
	add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
