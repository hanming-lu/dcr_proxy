# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

# Openssl
find_package(OpenSSL REQUIRED)

# Use the edger8r to generate C bindings from the EDL file.
add_custom_command(
  OUTPUT dcr_proxy_t.h dcr_proxy_t.c dcr_proxy_args.h
  DEPENDS ${CMAKE_SOURCE_DIR}/dcr_proxy.edl
  COMMAND
    openenclave::oeedger8r --trusted ${CMAKE_SOURCE_DIR}/dcr_proxy.edl
    --search-path ${OE_INCLUDEDIR} --search-path
    ${OE_INCLUDEDIR}/openenclave/edl/sgx)

include_directories(${CMAKE_SOURCE_DIR}/include)
set(enclave_source_files
  enc.cc
  ${CMAKE_SOURCE_DIR}/util/logging.cc
)
add_executable(enclave ${enclave_source_files}
                       ${CMAKE_CURRENT_BINARY_DIR}/dcr_proxy_t.c)

if (WIN32)
  maybe_build_using_clangw(enclave)
endif ()

target_compile_definitions(enclave PUBLIC OE_API_VERSION=2)

# Need for the generated file dcr_proxy_t.h
target_include_directories(enclave PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (LVI_MITIGATION MATCHES ControlFlow)
  # Helper to enable compiler options for LVI mitigation.
  apply_lvi_mitigation(enclave)
  # Link against LVI-mitigated libraries.
  target_link_libraries(
    enclave openenclave::oeenclave-lvi-cfg
    openenclave::oecrypto${OE_CRYPTO_LIB}-lvi-cfg openenclave::oelibc-lvi-cfg
    openenclave::oecryptoopenssl)
else ()
  target_link_libraries(
    enclave openenclave::oeenclave openenclave::oecrypto${OE_CRYPTO_LIB}
    openenclave::oelibcxx
    openenclave::oecryptoopenssl)
endif ()
