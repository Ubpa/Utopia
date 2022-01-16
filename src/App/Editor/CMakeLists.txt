Ubpa_AddTarget(
  TEST
  RET_TARGET_NAME app_editor
  MODE EXE
  LIB
    Ubpa::Utopia_Editor_Core
)

set_target_properties ( ${app_editor} PROPERTIES OUTPUT_NAME "UtopiaEngineEditor" CLEAN_DIRECT_OUTPUT 0 )
