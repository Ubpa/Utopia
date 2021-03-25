#pragma once

namespace Ubpa::Utopia {
	template<typename Func>
	void Serializer::RegisterComponentSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext&>);
		using ConstCmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstCmptPtr>);
		using ConstCmpt = std::remove_pointer_t<ConstCmptPtr>;
		static_assert(std::is_const_v<ConstCmpt>);
		using Cmpt = std::remove_const_t<ConstCmpt>;
		RegisterComponentSerializeFunction(
			TypeID_of<Cmpt>,
			[f = std::forward<Func>(func)](const void* p, SerializeContext& ctx) {
				f(reinterpret_cast<const Cmpt*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterComponentDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext&>);
		using CmptPtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<CmptPtr>);
		using Cmpt = std::remove_pointer_t<CmptPtr>;
		static_assert(!std::is_const_v<Cmpt>);
		RegisterComponentDeserializeFunction(
			TypeID_of<Cmpt>,
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext& ctx) {
				f(reinterpret_cast<Cmpt*>(p), jsonValueCmpt, ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterUserTypeSerializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 2);
		static_assert(std::is_same_v<At_t<ArgList, 1>, SerializeContext&>);
		using ConstUserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<ConstUserTypePtr>);
		using ConstUserType = std::remove_pointer_t<ConstUserTypePtr>;
		static_assert(std::is_const_v<ConstUserType>);
		using UserType = std::remove_const_t<ConstUserType>;
		RegisterUserTypeSerializeFunction(
			GetID<UserType>(),
			[f = std::forward<Func>(func)](const void* p, SerializeContext& ctx) {
				f(reinterpret_cast<const UserType*>(p), ctx);
			}
		);
	}

	template<typename Func>
	void Serializer::RegisterUserTypeDeserializeFunction(Func&& func) {
		using ArgList = FuncTraits_ArgList<Func>;
		static_assert(Length_v<ArgList> == 3);
		static_assert(std::is_same_v<At_t<ArgList, 1>, const rapidjson::Value&>);
		static_assert(std::is_same_v<At_t<ArgList, 2>, DeserializeContext&>);
		using UserTypePtr = At_t<ArgList, 0>;
		static_assert(std::is_pointer_v<UserTypePtr>);
		using UserType = std::remove_pointer_t<UserTypePtr>;
		static_assert(!std::is_const_v<UserType>);
		RegisterUserTypeDeserializeFunction(
			GetID<UserType>(),
			[f = std::forward<Func>(func)](void* p, const rapidjson::Value& jsonValueCmpt, DeserializeContext& ctx) {
				f(reinterpret_cast<UserType*>(p), jsonValueCmpt, ctx);
			}
		);
	}

	template<typename UserType>
	std::string Serializer::ToJSON(const UserType* obj) {
		static_assert(!std::is_void_v<UserType>);
		return ToJSON(TypeID_of<UserType>.GetValue(), obj);
	}
}
