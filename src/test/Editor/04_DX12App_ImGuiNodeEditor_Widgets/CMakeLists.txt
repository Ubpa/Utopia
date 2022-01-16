Ubpa_AddTarget(
  TEST
  MODE EXE
  INC
    "${PROJECT_SOURCE_DIR}/src/_deps/imgui"
  LIB
    Ubpa::Utopia_Editor_DX12App
    Ubpa::Utopia__deps_imgui_node_editor
)
