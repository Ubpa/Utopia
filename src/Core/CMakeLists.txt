Ubpa_DownloadFile(
	https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/stb_image.h
	${CMAKE_CURRENT_SOURCE_DIR}/_deps/stb_image.h
	SHA256 866095d4107665b4f2ac84e5aa25c7b4bd2f89c5408348c87fdec8891661cace
)

Ubpa_DownloadFile(
	https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/stb_image_write.h
	${CMAKE_CURRENT_SOURCE_DIR}/_deps/stb_image_write.h
	SHA256 cbd5f0ad7a9cf4468affb36354a1d2338034f2c12473cf1a8e32053cb6914a05
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
