Ubpa_AddTarget(
  TEST
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/App/DX12App"
  LIB
    Ubpa::DustEngine_Asset
  L_OPTION_INTERFACE
    /SUBSYSTEM:WINDOWS
)
