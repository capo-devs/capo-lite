add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
  capo::lite
  capo::compile-options
  gvdi::gvdi
)

target_sources(${PROJECT_NAME} PRIVATE
  player/app.cpp
  player/app.hpp
  player/controller.hpp
  player/input.cpp
  player/input.hpp
  player/main.cpp
  player/model.hpp
  player/view.cpp
  player/view.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  .
)
