project(capo-examples)

add_executable(${PROJECT_NAME}-sound)
target_link_libraries(${PROJECT_NAME}-sound PRIVATE
  capo::lite
  capo::compile-options
)
target_sources(${PROJECT_NAME}-sound PRIVATE sound.cpp)

add_executable(${PROJECT_NAME}-music)
target_link_libraries(${PROJECT_NAME}-music PRIVATE
  capo::lite
  capo::compile-options
)
target_sources(${PROJECT_NAME}-music PRIVATE music.cpp)

add_executable(${PROJECT_NAME}-interactive)
target_link_libraries(${PROJECT_NAME}-interactive PRIVATE
  capo::lite
  capo::compile-options
)
target_sources(${PROJECT_NAME}-interactive PRIVATE interactive.cpp)
