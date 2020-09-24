Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/ScriptSystem"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::ULuaPP_core
    Ubpa::DustEngine_Render
  DEFINE_PRIVATE
    _CRT_SECURE_NO_WARNINGS
  C_OPTION_PRIVATE
    /bigobj
)
