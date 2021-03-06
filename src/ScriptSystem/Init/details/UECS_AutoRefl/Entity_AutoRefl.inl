// This file is generated by Ubpa::USRefl::AutoRefl

#pragma once

#include <USRefl/USRefl.h>

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::UECS::Entity> :
    TypeInfoBase<Ubpa::UECS::Entity>
{
#ifdef UBPA_USREFL_NOT_USE_NAMEOF
    static constexpr char name[19] = "Ubpa::UECS::Entity";
#endif
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
        Field {TSTR(UMeta::constructor), WrapConstructor<Type(size_t, size_t)>()},
        Field {TSTR(UMeta::constructor), WrapConstructor<Type()>()},
        Field {TSTR("Idx"), &Type::Idx},
        Field {TSTR("Version"), &Type::Version},
        Field {TSTR("Invalid"), &Type::Invalid},
        Field {TSTR("Valid"), &Type::Valid},
        Field {TSTR("operator=="), &Type::operator==},
        Field {TSTR("operator<"), &Type::operator<},
    };
};

