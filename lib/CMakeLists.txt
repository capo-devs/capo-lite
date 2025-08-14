project(capo-lib VERSION ${capo_version})

message(STATUS "[capo]")

set(version_file "${CMAKE_CURRENT_BINARY_DIR}/include/capo/build_version.hpp")
configure_file(src/build_version.hpp.in "${version_file}" @ONLY)

add_library(${PROJECT_NAME})
add_library(capo::capo ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
  miniaudio::miniaudio
)

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/capo/*.hpp")

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS include
  FILES ${headers}
)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include"
  FILES "${version_file}"
)

target_sources(${PROJECT_NAME} PRIVATE
  src/capo.cpp
)
