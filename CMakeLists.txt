cmake_minimum_required(VERSION 3.23)

project(capo-lite VERSION "2.0.0")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

set(capo_is_top_level ${PROJECT_IS_TOP_LEVEL})

option(CAPO_BUILD_EXAMPLES "Build capo examples" ${capo_is_top_level})
option(CAPO_MA_DEBUG_OUTPUT "Enable miniaudio debug output" ${capo_is_top_level})

add_subdirectory(ext)

add_library(capo-warnings INTERFACE)
add_library(capo::warnings ALIAS capo-warnings)

if(capo_is_top_level)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(capo-warnings INTERFACE
      -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
    )
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(capo-warnings INTERFACE
      $<$<NOT:$<CONFIG:Debug>>:-Werror>
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
    target_compile_options(capo-warnings
      $<$<NOT:$<CONFIG:Debug>>:/WX>
    )
  endif()
endif()

add_subdirectory(lib)

if(CAPO_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
