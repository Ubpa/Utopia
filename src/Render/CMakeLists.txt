Ubpa_DownloadZip_Pro(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/Utopia/LTCTex.zip
  LTCTex.zip
  "${CMAKE_CURRENT_SOURCE_DIR}/_deps"
  SHA256 A389CBC501DA7FB24113EA4D2812C6B5D244EF6BBFC241DFFBB9527BAA76DC3C
)

Ubpa_DownloadZip_Pro(
  http://home.ustc.edu.cn/~ustczt/resources/Utopia/UShaderCompiler_deps_20210608.zip
  UShaderCompiler_deps_20210608.zip
  ${CMAKE_CURRENT_SOURCE_DIR}/UShaderCompiler/_deps
  SHA256 2EDB0AD656C2A5F0FAF05D02C4EE08C579D2BA16E4F13E45434AA0B3D2AF24DC
)

set(libs_private "")
set(defines_private "")

find_package(assimp QUIET)
if(TARGET assimp::assimp)
  list(APPEND libs_private assimp::assimp)
  list(APPEND defines_private UBPA_UTOPIA_USE_ASSIMP)
else()
  message(STATUS "[Utopia/Asset] not found assimp")
endif()

set(c_options_private "")
if(MSVC)
  list(APPEND c_options_private "/MP;/wd4244;/wd4804")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #
endif()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Render"
	${refls}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UDX12_core
    Ubpa::Utopia_Core
    Ubpa::UANTLR_runtime
    Ubpa::Utopia__deps_tinyobjloader
    Ubpa::USignal_core
  LIB_PRIVATE
    ${libs_private}
  C_OPTION_PRIVATE
    ${c_options_private}
  DEFINE_PRIVATE
    ${defines_private}
)
