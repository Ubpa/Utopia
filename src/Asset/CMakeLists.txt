Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Asset"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::URapidJSON_core
    Ubpa::DustEngine__deps_crossguid
)
