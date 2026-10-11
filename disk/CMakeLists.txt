add_library(grzdisk STATIC
  grzdisk.cc
  virtual_drive.cc
  dmk.cc
  dsk.cc
  jv1.cc
  jv3.cc
  td0.cc
  td0_compress.cc
)

target_include_directories(grzdisk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(grzdisk PUBLIC grzcommon)

add_library(cpmdrive STATIC
  cpmdrive/diskdef.cc
  cpmdrive/cpm_drive.cc
)
target_include_directories(cpmdrive PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cpmdrive)
target_link_libraries(cpmdrive PUBLIC grzdisk)

# The tool target cannot share the library's target name. The binary is still
# called grzdisk, matching the automake package.
add_executable(grzdisk_tool
  grzdisk.cc
  cpmfs.cc
)

set_target_properties(grzdisk_tool PROPERTIES OUTPUT_NAME grzdisk)
target_include_directories(grzdisk_tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(grzdisk_tool PRIVATE grzdisk grzcommon)
