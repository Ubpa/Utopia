cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DustEngine VERSION 0.0.1)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake()

Ubpa_InitProject()

Ubpa_AddDep(URapidJSON 0.0.1)
Ubpa_AddDep(USTL       0.1.1)
Ubpa_AddDep(UDP        0.7.1)
Ubpa_AddDep(UGM        0.6.11)
Ubpa_AddDep(ULuaPP     0.1.6)
Ubpa_AddDep(UECS       0.12.2)
Ubpa_AddDep(UDX12      0.0.8)

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/DustEngine/crossguid/crossguid_20200919.zip
  crossguid_20200919.zip
  SHA256 1BEE5F2424612E3EF3684D139D6038EF2420EAC7687BF210057E752EFE2A05D5
)

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/DustEngine/imgui/imgui_dock_1_78_0.zip
  imgui_dock_1_78_0.zip
  SHA256 E57BA9CD40CA2113BEF0BC1864C3B8BC0B9A97C39F50DA02AAF251CE0A168F0C
)

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(DIRECTORIES "include")
