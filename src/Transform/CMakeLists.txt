set(components
  Children
  LocalToParent
  LocalToWorld
  Parent
  Rotation
  RotationEuler
  Scale
  Translation
  WorldToLocal
)

set(refls "")

foreach(cmpt ${components})
  set(dst "${PROJECT_SOURCE_DIR}/include/DustEngine/Transform/Components/details/${cmpt}_AutoRefl.inl")
  Ubpa_USReflAutoRefl(
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Transform/Components/${cmpt}.h"
    ${dst}
  )
  list(APPEND refls ${dst})
endforeach()

Ubpa_AddTarget(
  MODE STATIC
  SOURCE
    "${PROJECT_SOURCE_DIR}/include/DustEngine/Transform"
    ${refls}
  INC
    "${PROJECT_SOURCE_DIR}/include"
  LIB
    Ubpa::UGM_core
    Ubpa::UECS_core
    Ubpa::USRefl_core
)
