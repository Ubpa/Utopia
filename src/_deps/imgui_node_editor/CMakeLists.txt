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
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui_node_editor"
  INC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui"
  INC_PRIVATE
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui_node_editor"
    "${PROJECT_SOURCE_DIR}/src/_deps/imgui"
  C_OPTION_PRIVATE
    ${c_options_private}
  LIB
    Ubpa::Utopia__deps_imgui
)
