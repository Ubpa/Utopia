cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DustEngine VERSION 0.0.1)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake()

Ubpa_InitProject()

Ubpa_AddDep(URapidJSON 0.0.1)
Ubpa_AddDep(USTL       0.1.1)
Ubpa_AddDep(UDP        0.7.1)
Ubpa_AddDep(UGM        0.6.9)
Ubpa_AddDep(ULuaPP     0.1.5)
Ubpa_AddDep(UECS       0.12.1)
Ubpa_AddDep(UDX12      0.0.8)

Ubpa_DownloadZip(
	https://cdn.jsdelivr.net/gh/Ubpa/UData@master/DustEngine/crossguid/crossguid_20200809.zip
	crossguid_20200809.zip
	SHA256 567663B30C1CEB24C8C9D815C1D60579A7CA9D3D3888702858EB2FD672E429B5
)

Ubpa_DownloadZip(
	https://cdn.jsdelivr.net/gh/Ubpa/UData@master/DustEngine/imgui/imgui_1_78.zip
	imgui_1_78.zip
	SHA256 7620326BE62F5748B2547462496C6A6552ED61BA46A71E744215FF17C1022F25
)

Ubpa_AddSubDirsRec(include)
Ubpa_AddSubDirsRec(src)

Ubpa_Export(DIRECTORIES "include")
