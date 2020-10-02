#pragma once

#include <UECS/World.h>

#include <UDP/Visitor/cVisitor.h>
#include <UDP/Visitor/ncVisitor.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <string>

namespace Ubpa::Utopia {
	class Serializer {
	public:
		static Serializer& Instance() {
			static Serializer instance;
			return instance;
		}

		struct Key {
			static constexpr const char ENTITY_MNGR[] = "__ENTITY_MNGR";
			static constexpr const char ENTITIES[] = "__ENTITIES";
			static constexpr const char INDEX[] = "__INDEX";
			static constexpr const char COMPONENTS[] = "__COMPONENTS";
			static constexpr const char TYPE[] = "__TYPE";
			static constexpr const char NAME[] = "__NAME";
			static constexpr const char CONTENT[] = "__CONTENT";
			static constexpr const char NOT_SUPPORT[] = "__NOT_SUPPORT";
			static constexpr const char KEY[] = "__KEY";
			static constexpr const char MAPPED[] = "__MAPPED";
		};

		using JSONWriter = rapidjson::Writer<rapidjson::StringBuffer>;
		struct SerializeContext {
			SerializeContext(const Visitor<void(const void*, SerializeContext&)>& s) :serializer{ s } { writer.Reset(sb); }
			JSONWriter writer;
			rapidjson::StringBuffer sb;
			const Visitor<void(const void*, SerializeContext&)>& serializer;
		};
		using SerializeFunc = std::function<void(const void*, SerializeContext&)>;
		using EntityIdxMap = std::unordered_map<size_t, UECS::Entity>;
		struct DeserializeContext {
			const EntityIdxMap& entityIdxMap;
			const Visitor<void(void*, const rapidjson::Value&, DeserializeContext&)>& deserializer;
		};
		using DeserializeFunc = std::function<void(void*, const rapidjson::Value&, DeserializeContext&)>;

		void RegisterComponentSerializeFunction(UECS::CmptType, SerializeFunc);
		void RegisterComponentDeserializeFunction(UECS::CmptType, DeserializeFunc);

		template<typename Func>
		void RegisterComponentSerializeFunction(Func&& func);
		template<typename Func>
		void RegisterComponentDeserializeFunction(Func&& func);

		template<typename... Cmpts>
		void RegisterComponentSerializeFunction();
		template<typename... Cmpts>
		void RegisterComponentDeserializeFunction();

		// register Cmpts' serialize and deserialize function
		template<typename... Cmpts>
		void RegisterComponents();

		void RegisterUserTypeSerializeFunction(size_t id, SerializeFunc);
		void RegisterUserTypeDeserializeFunction(size_t id, DeserializeFunc);

		template<typename Func>
		void RegisterUserTypeSerializeFunction(Func&& func);
		template<typename Func>
		void RegisterUserTypeDeserializeFunction(Func&& func);

		template<typename... UserTypes>
		void RegisterUserTypeSerializeFunction();
		template<typename... UserTypes>
		void RegisterUserTypeDeserializeFunction();

		// register UserTypes' serialize and deserialize function
		template<typename... UserTypes>
		void RegisterUserTypes();

		std::string ToJSON(const UECS::World*);
		bool ToWorld(UECS::World*, std::string_view json);

		std::string ToJSON(size_t ID, const void* obj);
		template<typename UserType>
		std::string ToJSON(const UserType* obj);
		bool ToUserType(std::string_view json, size_t ID, void* obj);
		template<typename UserType>
		bool ToUserType(std::string_view json, UserType* obj);
	private:
		// core functions
		void RegisterSerializeFunction(size_t id, SerializeFunc);
		void RegisterDeserializeFunction(size_t id, DeserializeFunc);

		Serializer();
		~Serializer();

		struct Impl;
		Impl* pImpl;
	};
}

#include "details/Serializer.inl"
