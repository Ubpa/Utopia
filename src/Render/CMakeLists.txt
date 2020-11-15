set(components
  Camera
  MeshFilter
  MeshRenderer
  Skybox
)

set(refls "")

foreach(cmpt ${components})
  set(dst "${PROJECT_SOURCE_DIR}/include/Utopia/Render/Components/details/${cmpt}_AutoRefl.inl")
  Ubpa_USReflAutoRefl(
    "${PROJECT_SOURCE_DIR}/include/Utopia/Render/Components/${cmpt}.h"
    ${dst}
  )
  list(APPEND refls ${dst})
endforeach()

Ubpa_DownloadZip_Pro(
  https://cdn.jsdelivr.net/gh/Ubpa/UData@master/Utopia/LTCTex.zip
  LTCTex.zip
  "${CMAKE_CURRENT_SOURCE_DIR}/_deps"
  SHA256 A389CBC501DA7FB24113EA4D2812C6B5D244EF6BBFC241DFFBB9527BAA76DC3C
)

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/Utopia/Render"
	${refls}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UDX12_core
    Ubpa::Utopia_Core
)
