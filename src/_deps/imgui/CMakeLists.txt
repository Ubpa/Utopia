set(c_options_private "")
if(MSVC)
  list(APPEND c_options_private "/MP")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #
endif()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  INC_PRIVATE
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui"
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui/misc/cpp"
  C_OPTION_PRIVATE
    ${c_options_private}
  LIB
    Ubpa::UGM_core
)
