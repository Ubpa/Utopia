Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Editor/Core"
  LIB
    Ubpa::Utopia_Editor_DX12App
    Ubpa::Utopia__deps_imgui_node_editor
  C_OPTION
    /bigobj
)
