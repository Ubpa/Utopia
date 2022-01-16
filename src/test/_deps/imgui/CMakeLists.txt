Ubpa_AddTarget(
  TEST
  MODE EXE
  INC
    "${PROJECT_SOURCE_DIR}/include/_deps/imgui"
  LIB
    Ubpa::Utopia__deps_imgui
    "d3d12.lib"
    "d3dcompiler.lib"
    "dxgi.lib"
)
