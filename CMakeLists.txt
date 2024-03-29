cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(Utopia VERSION 0.0.6)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake(VERSION 0.6.4)

Ubpa_InitProject()

set(Ubpa_USRefl_Build_AutoRefl TRUE CACHE BOOL "use auto refl" FORCE)

Ubpa_AddDep(URapidJSON 0.0.4)
Ubpa_AddDep(UVisitor   0.1.0)
Ubpa_AddDep(UGM        0.7.6)
Ubpa_AddDep(UECS       0.17.0)
Ubpa_AddDep(UDX12      0.2.1)
Ubpa_AddDep(UDRefl     0.11.0)
Ubpa_AddDep(UANTLR     0.3.0)
Ubpa_AddDep(USignal    0.3.2)

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/crossguid_20200919.zip
  crossguid_20200919.zip
  SHA256 1BEE5F2424612E3EF3684D139D6038EF2420EAC7687BF210057E752EFE2A05D5
)

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/imgui_dock_1_86_1.zip
  imgui_dock_1_86_1.zip
  SHA256 A5836CAA484B236AA8F6489F9FAD3D1E861F000554D843ACEB57E6A1F4DCFC14
)

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/imgui_node_editor_20220116_d79ebde.zip
  imgui_node_editor_20220116_d79ebde.zip
  SHA256 764E0DBB311B6D137CE47408670C10100062A1534EE1804E0C0CF5B9488AC825
)

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/Utopia_spdlog_1_9_2_20211227.zip
  Utopia_spdlog_1_9_2_20211227.zip
  SHA256 3EA64433EE71EEE0F4EC02D877E137DC7C0CD2DDEECE3610E6B06C961895BA84
)

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/assets_internal_pngs_20220219.zip
  assets_internal_pngs_20220219.zip
  SHA256 39e629196f462c6de75c964203e068bde3d5120d72ad4d9a921d7b9f32dd7df6
)

if(Ubpa_BuildTest_Utopia)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    message(NOTICE "GTest Found")
  else()
    message(NOTICE "GTest not Found, so we ignore some targets depending on GTest")
  endif()
endif()

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(
  TARGET
  DIRECTORIES
    "include"
)
