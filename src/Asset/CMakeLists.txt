set(libs "")
set(defines "")

find_package(assimp QUIET)
if(TARGET assimp::assimp)
  list(APPEND libs assimp::assimp)
  list(APPEND defines UBPA_DUSTENGINE_USE_ASSIMP)
else()
  message(STATUS "[DustEngine/Asset] not found assimp")
endif()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Asset"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::URapidJSON_core
    Ubpa::DustEngine__deps_crossguid
    Ubpa::DustEngine__deps_tinyobjloader
    Ubpa::DustEngine_ScriptSystem
    ${libs}
  DEFINE_PRIVATE
    ${defines}
)
