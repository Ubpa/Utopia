set(libs "")
set(defines "")

find_package(assimp QUIET)
if(TARGET assimp::assimp)
  list(APPEND libs assimp::assimp)
  list(APPEND defines UBPA_DUSTENGINE_USE_ASSIMP)
else()
  message(STATUS "[Utopia/Asset] not found assimp")
endif()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Asset"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::URapidJSON_core
    Ubpa::Utopia__deps_crossguid
    Ubpa::Utopia__deps_tinyobjloader
    Ubpa::Utopia_ScriptSystem
    ${libs}
  DEFINE_PRIVATE
    ${defines}
)
