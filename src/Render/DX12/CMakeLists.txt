Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Render/DX12"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UDX12_core
    Ubpa::DustEngine_Core
)
