cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DustEngine VERSION 0.0.1)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake()

Ubpa_InitProject()

Ubpa_AddDep(URapidJSON 0.0.1)
Ubpa_AddDep(USTL       0.1.1)
Ubpa_AddDep(UDP        0.7.1)
Ubpa_AddDep(UGM        0.6.10)
Ubpa_AddDep(ULuaPP     0.1.6)
Ubpa_AddDep(UECS       0.12.2)
Ubpa_AddDep(UDX12      0.0.8)

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/DustEngine/crossguid/crossguid_20200919.zip
  crossguid_20200919.zip
  SHA256 1BEE5F2424612E3EF3684D139D6038EF2420EAC7687BF210057E752EFE2A05D5
)

Ubpa_DownloadZip(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/DustEngine/imgui/imgui_1_78_3.zip
  imgui_1_78_3.zip
  SHA256 8E90BCF8218B009BC4C9DAAC3DDD86753A72EC6EE40791FF2DFDD2D5C0811B77
)

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(DIRECTORIES "include")
