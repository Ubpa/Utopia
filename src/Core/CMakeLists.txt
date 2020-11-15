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

set(components
  Children
  Input
  LocalToParent
  LocalToWorld
  Name
  Parent
  Roamer
  Rotation
  RotationEuler
  Scale
  NonUniformScale
  Translation
  WorldTime
  WorldToLocal
)

set(refls "")

foreach(cmpt ${components})
  set(dst "${PROJECT_SOURCE_DIR}/include/Utopia/Core/Components/details/${cmpt}_AutoRefl.inl")
  Ubpa_USReflAutoRefl(
    "${PROJECT_SOURCE_DIR}/include/Utopia/Core/Components/${cmpt}.h"
    ${dst}
  )
  list(APPEND refls ${dst})
endforeach()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Core"
	${refls}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::USTL_core
    Ubpa::USRefl_core
    Ubpa::UGM_core
    Ubpa::UDP_core
    Ubpa::UECS_core
    Ubpa::UDX12_core
    Ubpa::Utopia__deps_imgui
    Ubpa::Utopia__deps_spdlog
  DEFINE
    NOMINMAX
)
