add_library(grzcommon STATIC
  binfile.cc
  cassette.cc
  cmdargs.cc
  misc.cc
)

target_include_directories(grzcommon
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${GRATZE_CEREAL_INCLUDE}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
