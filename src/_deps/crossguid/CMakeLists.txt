Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/_deps/crossguid"
  INC
    "${PROJECT_SOURCE_DIR}/include"
  INC_PRIVATE
    "${PROJECT_SOURCE_DIR}/include/_deps"
  DEFINE_PRIVATE
    GUID_WINDOWS
)
