Ubpa_DownloadFile(
	https://ubpa.github.io/UData/common/include/stb_image.h
	${CMAKE_CURRENT_SOURCE_DIR}/_deps/stb_image.h
	SHA256 281C4F378083E1F195CD06FB98F1EC7EB9749353EB19C915F17C2AF46ABBAFEE
)

Ubpa_DownloadFile(
	https://ubpa.github.io/UData/common/include/stb_image_write.h
	${CMAKE_CURRENT_SOURCE_DIR}/_deps/stb_image_write.h
	SHA256 F34D6E55C7BB69C98BF66F191A7636A0B022047BCE7C7B16A9A9F356046FDC8A
)

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Core"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UGM_core
    Ubpa::UDP_core
)
