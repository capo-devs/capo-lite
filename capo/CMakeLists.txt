option(CAPO_PCH "Use pre-compiled headers for capo" ON)

add_library(capo-compile-options INTERFACE)
add_library(capo::compile-options ALIAS capo-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(capo-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()

add_library(${PROJECT_NAME})
add_library(capo::lite ALIAS ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
  $<$<BOOL:${CAPO_USE_OPENAL}>:CAPO_USE_OPENAL>
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_link_libraries(${PROJECT_NAME} PRIVATE
  capo::ext
  capo::compile-options
)

target_include_directories(${PROJECT_NAME}
  PUBLIC include "${CMAKE_CURRENT_BINARY_DIR}/include"
  PRIVATE src
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/build_version.hpp.in")
  configure_file(src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/capo/build_version.hpp" @ONLY)
endif()

if(CMAKE_GENERATOR MATCHES "^(Visual Studio)")
  target_compile_options(${PROJECT_NAME} INTERFACE /MP)
endif()

if(CAPO_PCH)
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    <algorithm>
    <atomic>
    <chrono>
    <filesystem>
    <functional>
    <future>
    <memory>
    <mutex>
    <optional>
    <span>
    <string>
    <unordered_map>
    <unordered_set>
    <variant>
    <vector>
  )
endif()

include(header_list.cmake)
target_sources(${PROJECT_NAME} PRIVATE ${capo_headers})
add_subdirectory(src)
