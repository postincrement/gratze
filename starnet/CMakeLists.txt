add_library(starnet_udp STATIC udp_port.cc)
target_include_directories(starnet_udp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(starnet-server
  main.cc
  starnet_server.cc
  starnet_session.cc
  udp_port.cc
)
target_include_directories(starnet-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(starnet-server PRIVATE cpmdrive)
set_target_properties(starnet-server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/starnet
)
