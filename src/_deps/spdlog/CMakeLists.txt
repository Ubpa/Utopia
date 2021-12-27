find_package(Threads REQUIRED)

set(SPDLOG_VERSION 1.9.2)
set(SPDLOG_VERSION_MAJOR 1)

set(c_options_private "")
if(MSVC)
  list(APPEND c_options_private "/MP")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #
endif()

# use fmt as default
# build static as default

Ubpa_AddTarget(
  MODE STATIC
  RET_TARGET_NAME spdlog
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/_deps/spdlog"
  DEFINE
    SPDLOG_COMPILED_LIB
  C_OPTION_PRIVATE
    ${c_options_private}
  INC
    "${PROJECT_SOURCE_DIR}/include/_deps"
  LIB
    Threads::Threads
)

set_target_properties(${spdlog} PROPERTIES VERSION ${SPDLOG_VERSION} SOVERSION ${SPDLOG_VERSION_MAJOR})

if(COMMAND target_precompile_headers)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pch.h.in ${PROJECT_BINARY_DIR}/spdlog_pch.h @ONLY)
    target_precompile_headers(${spdlog} PRIVATE ${PROJECT_BINARY_DIR}/spdlog_pch.h)
endif()
