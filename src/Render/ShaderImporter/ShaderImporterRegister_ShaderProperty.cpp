#include "ShaderImporterRegisterImpl.h"

#include <Utopia/Render/Shader.h>

using namespace Ubpa::Utopia;
using namespace Ubpa::UDRefl;

namespace Ubpa::Utopia::details {
	template<typename T, std::size_t... Ns>
	void simple_register_variant_alternatives(ReflMngr& mngr, std::index_sequence<Ns...>) {
		(Ubpa::UDRefl::details::register_variant_ctor_assign<T, Ns>(mngr), ...);
	}
}

template<>
struct Ubpa::UDRefl::details::TypeAutoRegister<ShaderPropertyVariant> {
	static auto run(UDRefl::ReflMngr& mngr) {
		using T = ShaderPropertyVariant;
		mngr.AddConstructor<T>();
		mngr.AddConstructor<T, const T&>();
		mngr.AddConstructor<T, T&&>();
		mngr.AddDestructor<T>();

		mngr.AddMemberMethod(NameIDRegistry::Meta::operator_assignment, [](T& lhs, const T& rhs) -> T& { return lhs = rhs; });
		mngr.AddMemberMethod(NameIDRegistry::Meta::operator_assignment, [](T& lhs, T&& rhs) -> T& { return lhs = std::move(rhs); });

		static_assert(IsVariant<T>);

		mngr.AddMemberMethod(NameIDRegistry::Meta::variant_index, [](const T& t) { return static_cast<std::size_t>(t.index()); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::variant_valueless_by_exception, [](const T& t) { return static_cast<bool>(t.valueless_by_exception()); });
		mngr.AddStaticMethod(Type_of<T>, NameIDRegistry::Meta::variant_size, []() { return static_cast<std::size_t>(std::variant_size_v<T>); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::get, [](T& t, const std::size_t& i) { return runtime_get<std::variant_size>(t, i); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::get, [](const T& t, const std::size_t& i) { return runtime_get<std::variant_size>(t, i); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::holds_alternative, [](const T& t, const Type& type) { return runtime_variant_holds_alternative(t, type); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::get, [](T& t, const Type& type) { return runtime_get<std::variant_size, std::variant_alternative>(t, type); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::get, [](const T& t, const Type& type) { return runtime_get<std::variant_size, std::variant_alternative>(t, type); });
		mngr.AddStaticMethod(Type_of<T>, NameIDRegistry::Meta::variant_alternative, [](const std::size_t& i) { return runtime_variant_alternative<T>(i); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::variant_visit_get, [](T& t) { return runtime_get<std::variant_size>(t, t.index()); });
		mngr.AddMemberMethod(NameIDRegistry::Meta::variant_visit_get, [](const T& t) { return runtime_get<std::variant_size>(t, t.index()); });
		Ubpa::Utopia::details::simple_register_variant_alternatives<T>(mngr, std::make_index_sequence<std::variant_size_v<T>>{});

		mngr.AddTypeAttr(Type_of<T>, mngr.MakeShared(Type_of<ContainerType>, TempArgsView{ ContainerType::Variant }));

		Mngr.RegisterType<SharedVar<Texture2D>>();
		Mngr.RegisterType<SharedVar<TextureCube>>();
	}
};

void Ubpa::Utopia::details::ShaderImporterRegister_ShaderProperty() {
	UDRefl::Mngr.RegisterType<ShaderProperty>();
	UDRefl::Mngr.AddField<&ShaderProperty::value>("value");
}
