add_library(grzcommon STATIC
  binfile.cc
  byte_stream.cc
  cassette.cc
  cmdargs.cc
  misc.cc
)

# https:// byte streams. Without OpenSSL that scheme returns Unsupported.
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
  target_compile_definitions(grzcommon PRIVATE GRZ_HAVE_OPENSSL)
  # Static consumers (gratze, starnet-server) have to link the TLS symbols.
  target_link_libraries(grzcommon PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

target_include_directories(grzcommon
  PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${GRATZE_CEREAL_INCLUDE}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
