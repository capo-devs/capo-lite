project(capo-player)

include(FetchContent)
FetchContent_Declare(
  gvdi
  GIT_REPOSITORY https://github.com/karnkaul/gvdi
  GIT_TAG 77db1bd
)
FetchContent_MakeAvailable(gvdi)

add_subdirectory(src)
