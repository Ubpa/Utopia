cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(Utopia VERSION 0.0.5)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake(VERSION 0.6.4)

Ubpa_InitProject()

set(Ubpa_USRefl_Build_AutoRefl TRUE CACHE BOOL "use auto refl" FORCE)

Ubpa_AddDep(URapidJSON 0.0.4)
Ubpa_AddDep(UVisitor   0.1.0)
Ubpa_AddDep(UGM        0.7.4)
Ubpa_AddDep(UECS       0.16.1)
Ubpa_AddDep(UDX12      0.1.2)
Ubpa_AddDep(UDRefl     0.10.5)
Ubpa_AddDep(UANTLR     0.3.0)
Ubpa_AddDep(USignal    0.3.1)

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/Utopia/crossguid/crossguid_20200919.zip
  crossguid_20200919.zip
  SHA256 1BEE5F2424612E3EF3684D139D6038EF2420EAC7687BF210057E752EFE2A05D5
)

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/Utopia/imgui/imgui_dock_1_78_0.zip
  imgui_dock_1_78_0.zip
  SHA256 E57BA9CD40CA2113BEF0BC1864C3B8BC0B9A97C39F50DA02AAF251CE0A168F0C
)

Ubpa_DownloadZip(
  https://udata-1308066321.cos.ap-guangzhou.myqcloud.com/spdlog_1_9_0_20211222.zip
  spdlog_1_9_0_20211222.zip
  SHA256 2618A0D0D0A5BCAB6DEE36E88262F53AA54EAD3F361472A1815C1A00F9DF8D5C
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
