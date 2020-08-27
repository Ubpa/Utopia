Ubpa_AddTarget(
  TEST
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/App/D3DApp"
  LIB
    Ubpa::DustEngine_Render_DX12
    Ubpa::DustEngine_Asset
  L_OPTION_INTERFACE
    /SUBSYSTEM:WINDOWS
)
