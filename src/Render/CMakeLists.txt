set(components
  Camera
  MeshFilter
  MeshRenderer
  Skybox
  Light
)

set(refls "")

foreach(cmpt ${components})
  set(dst "${PROJECT_SOURCE_DIR}/include/DustEngine/Render/Components/details/${cmpt}_AutoRefl.inl")
  Ubpa_USReflAutoRefl(
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Render/Components/${cmpt}.h"
    ${dst}
  )
  list(APPEND refls ${dst})
endforeach()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Render"
	${refls}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UDX12_core
    Ubpa::DustEngine_Core
)
