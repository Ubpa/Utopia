Ubpa_DownloadFile(
	https://cdn.jsdelivr.net/gh/Ubpa/UData@master/common/include/stb_image.h
	${CMAKE_CURRENT_SOURCE_DIR}/_deps/stb_image.h
	SHA256 281C4F378083E1F195CD06FB98F1EC7EB9749353EB19C915F17C2AF46ABBAFEE
)

Ubpa_DownloadFile(
	https://cdn.jsdelivr.net/gh/Ubpa/UData@master/common/include/stb_image_write.h
	${CMAKE_CURRENT_SOURCE_DIR}/_deps/stb_image_write.h
	SHA256 F34D6E55C7BB69C98BF66F191A7636A0B022047BCE7C7B16A9A9F356046FDC8A
)

set(c_options_private "")
if(MSVC)
  list(APPEND c_options_private "/MP;/wd4244")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #
endif()

set(c_options "")
if(MSVC)
  list(APPEND c_options "/wd5222")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #
endif()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Core"
	${refls}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UGM_core
    Ubpa::UVisitor_core
    Ubpa::UDRefl_core
    Ubpa::UECS_core
    Ubpa::UDX12_core
    Ubpa::URapidJSON_core
    Ubpa::Utopia__deps_imgui
    Ubpa::Utopia__deps_spdlog
    Ubpa::Utopia__deps_crossguid
  C_OPTION
    ${c_options}
  C_OPTION_PRIVATE
    ${c_options_private}
  DEFINE
    NOMINMAX
    UBPA_UDREFL_INCLUDE_ALL_STD_NAME
)
