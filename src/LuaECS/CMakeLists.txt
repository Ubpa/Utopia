Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/LuaECS"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::ULuaPP_core
    Ubpa::UECS_core
)
