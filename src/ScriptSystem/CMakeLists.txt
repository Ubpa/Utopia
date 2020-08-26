Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/ScriptSystem"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::ULuaPP_core
    Ubpa::UECS_core
    Ubpa::DustEngine_Core
  DEFINE_PRIVATE
    _CRT_SECURE_NO_WARNINGS
  C_OPTION_PRIVATE
    /bigobj
)
