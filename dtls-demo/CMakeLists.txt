cmake_minimum_required(VERSION 3.13)

project(dtls_demo C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SSL REQUIRED openssl)
pkg_check_modules(CRYPTO REQUIRED libcrypto)

add_executable(dtls_demo server2.c)
set_target_properties(dtls_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
        SUFFIX ".app")
target_include_directories(dtls_demo PUBLIC ${SSL_INCLUDE_DIRS} ${CRYPTO_INCLUDE_DIRS})
target_link_libraries(dtls_demo ${SSL_LIBRARIES} ${CRYPTO_LIBRARIES} dl pthread)