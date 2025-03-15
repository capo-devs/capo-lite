project(capo-ext)

find_package(Threads)

file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "[dr-libs]")
add_subdirectory(src/dr-libs)

if(CAPO_USE_OPENAL)
  set(ALSOFT_BACKEND_OBOE OFF)
  set(ALSOFT_BACKEND_OPENSL OFF)
  set(ALSOFT_BACKEND_SDL2 OFF)
  set(ALSOFT_NO_CONFIG_UTIL ON)
  set(ALSOFT_UTILS OFF)
  set(ALSOFT_EXAMPLES OFF)
  set(ALSOFT_INSTALL OFF)
  set(ALSOFT_INSTALL_CONFIG OFF)
  set(ALSOFT_INSTALL_HRTF_DATA OFF)
  set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF)
  set(ALSOFT_INSTALL_EXAMPLES OFF)
  set(ALSOFT_INSTALL_UTILS OFF)
  set(ALSOFT_UPDATE_BUILD_VERSION OFF)

  if(NOT CAPO_OPENAL_DLL)
    set(LIBTYPE STATIC)
  endif()

  message(STATUS "[openal-soft]")
  add_subdirectory(src/openal-soft)
  add_library(${PROJECT_NAME}::OpenAL ALIAS OpenAL)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(alsoft.fmt PUBLIC -w)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(alsoft.fmt PUBLIC /wd4244 /wd4267)
  endif()
endif()

add_library(${PROJECT_NAME} INTERFACE)
add_library(capo::ext ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} INTERFACE
  dr-libs::dr-libs
  $<$<BOOL:${CAPO_USE_OPENAL}>:OpenAL::OpenAL>
  Threads::Threads
)
