Ubpa_DownloadZip_Pro(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/Utopia/LTCTex.zip
  LTCTex.zip
  "${CMAKE_CURRENT_SOURCE_DIR}/_deps"
  SHA256 A389CBC501DA7FB24113EA4D2812C6B5D244EF6BBFC241DFFBB9527BAA76DC3C
)

Ubpa_DownloadZip_Pro(
  http://home.ustc.edu.cn/~ustczt/resources/Utopia/UShaderCompiler_deps_20210325.zip
  UShaderCompiler_deps_20210325.zip
  ${CMAKE_CURRENT_SOURCE_DIR}/UShaderCompiler/_deps
  SHA256 D43B70DA3B92D5BA3875243527EDA4609E0622D152CE97C08306240DC98E8729
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
  LIB_PRIVATE
    ${libs_private}
  C_OPTION_PRIVATE
    ${c_options_private}
  DEFINE_PRIVATE
    ${defines_private}
)
