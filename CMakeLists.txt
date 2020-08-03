cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(DustEngine VERSION 0.0.1)
message(STATUS "[Project] ${PROJECT_NAME}")

include(cmake/InitUCMake.cmake)
Ubpa_InitUCMake()

Ubpa_InitProject()

Ubpa_AddDep(URapidJSON  0.0.1)
Ubpa_AddDep(USTL        0.1.1)
Ubpa_AddDep(UDP         0.7.0)
Ubpa_AddDep(UGM         0.6.7)
Ubpa_AddDep(ULuaPP      0.1.2)
Ubpa_AddDep(UECS        0.10.4)
Ubpa_AddDep(UDXRenderer 0.0.3)

Ubpa_AddSubDirsRec(src)

Ubpa_Export(DIRECTORIES "include")
