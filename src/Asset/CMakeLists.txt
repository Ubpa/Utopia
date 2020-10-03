# ushader
set(target_dir "${PROJECT_SOURCE_DIR}/src/Asset/ShaderCompiler/_deps")
if(NOT EXISTS ${target_dir})
  Ubpa_ANTLR4Generate(
    GEN_VISITOR
	GEN_LISTENER
	G4 "${PROJECT_SOURCE_DIR}/cmake/Shader.g4"
	NAMESPACE Ubpa::Utopia::details
	DIR ${target_dir}
  )
endif()

# deps

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
    "${PROJECT_SOURCE_DIR}/cmake/Shader.g4"
	${ushader_parser_files}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::URapidJSON_core
    Ubpa::Utopia__deps_crossguid
    Ubpa::Utopia__deps_tinyobjloader
    Ubpa::Utopia_ScriptSystem
    ${libs}
  LIB_PRIVATE
    Ubpa::UANTLR_runtime
  DEFINE_PRIVATE
    ${defines}
)
