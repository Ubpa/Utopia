#pragma once

namespace Ubpa::Utopia {
	template<typename Func>
	void Serializer::RegisterSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext&>);
		using ConstUserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstUserTypePtr>);
		using ConstUserType = std::remove_pointer_t<ConstUserTypePtr>;
		static_assert(std::is_const_v<ConstUserType>);
		using UserType = std::remove_const_t<ConstUserType>;
		RegisterSerializeFunction(
			TypeID_of<UserType>,
			[f = std::forward<Func>(func)](const void* p, SerializeContext& ctx) {
				f(reinterpret_cast<const UserType*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext&>);
		using UserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<UserTypePtr>);
		using UserType = std::remove_pointer_t<UserTypePtr>;
		static_assert(!std::is_const_v<UserType>);
		RegisterDeserializeFunction(
			TypeID_of<UserType>,
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& value, DeserializeContext& ctx) {
				f(reinterpret_cast<UserType*>(p), value, ctx);
			}
		);
	}

	template<typename UserType>
	std::string Serializer::Serialize(const UserType* obj) {
		static_assert(!std::is_void_v<UserType>);
		return Serialize(TypeID_of<UserType>.GetValue(), obj);
	}
}
