Ubpa_DownloadZip_Pro(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/Utopia/ushader/UShaderCompiler_deps_20201122.zip
  UShaderCompiler_deps_20201122.zip
  ${CMAKE_CURRENT_SOURCE_DIR}/UShaderCompiler/_deps
  SHA256 2B5531D579A1C665A7D7EA1FE396D8F62D6043CCB2A88890ED42FD19B858AE0D
)

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

# target
Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Asset"
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
