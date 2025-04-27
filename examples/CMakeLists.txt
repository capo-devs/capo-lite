project(capo-examples)

message(STATUS "[${PROJECT_NAME}]")

function(add_example ID)
  add_executable(capo-${ID})

  target_link_libraries(capo-${ID} PRIVATE
    capo::capo
    capo::warnings
  )

  target_sources(capo-${ID} PRIVATE
    src/${ID}.cpp
  )
endfunction()

add_example(basic)
add_example(stream)
