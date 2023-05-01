add_subdirectory(detail)

if(CAPO_USE_OPENAL)
  add_subdirectory(openal)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  device.cpp
  duration.cpp
  pcm.cpp
  sound_source.cpp
  stream_source.cpp
  stream.cpp
)
