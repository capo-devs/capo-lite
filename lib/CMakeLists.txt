project(capo-lib)

message(STATUS "[capo]")

configure_file(src/version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/capo/version.hpp" @ONLY)

add_library(${PROJECT_NAME})
add_library(capo::capo ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
  miniaudio::miniaudio
  capo::warnings
)

file(GLOB_RECURSE headers LIST_DIRECTORIES false "include/capo/*.hpp")

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS include
  FILES ${headers}
)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include"
  FILES "${CMAKE_CURRENT_BINARY_DIR}/include/capo/version.hpp"
)

target_sources(${PROJECT_NAME} PRIVATE
  src/capo.cpp
)
