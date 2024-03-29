#pragma once

#include <UECS/UECS.hpp>

#include <UVisitor/cVisitor.h>
#include <UVisitor/ncVisitor.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <UDRefl/UDRefl.hpp>

#include <string>

namespace Ubpa::Utopia {
	class Serializer {
	public:
		static Serializer& Instance() {
			static Serializer instance;
			return instance;
		}

		struct Key {
			static constexpr const char EntityMngr[] = "__EntityMngr";
			static constexpr const char Entities[] = "__Entities";
			static constexpr const char Index[] = "__Index";
			static constexpr const char Components[] = "__Components";
			static constexpr const char Type[] = "__Type";
			static constexpr const char Data[] = "__Data";
			static constexpr const char NotSupport[] = "__NotSupport";
			static constexpr const char Name[] = "__Name";
			static constexpr const char Guid[] = "__Guid";
		};

		//
		// Serialize
		//////////////

		using JSONWriter = rapidjson::Writer<rapidjson::StringBuffer>;
		struct SerializeContext {
			SerializeContext(const Visitor<void(const void*, SerializeContext&)>& s) :serializer{ s } { writer.Reset(sb); }
			JSONWriter writer;
			rapidjson::StringBuffer sb;
			const Visitor<void(const void*, SerializeContext&)>& serializer;
		};
		using SerializeFunc = std::function<void(const void*, SerializeContext&)>;

		void RegisterSerializeFunction(TypeID id, SerializeFunc);

		template<typename Func> // (const T*, SerializeContext&)
		void RegisterSerializeFunction(Func&& func);

		std::string Serialize(const UECS::World*);
		std::string Serialize(const UECS::World*, std::span<UECS::Entity> entities);
		std::string Serialize(size_t ID, const void* obj);
		std::string Serialize(UDRefl::ObjectView obj);
		template<typename UserType>
		std::string Serialize(const UserType* obj);

		static void SerializeRecursion(UDRefl::ObjectView obj, SerializeContext& ctx);

		//
		// Deserialize
		////////////////

		using EntityIdxMap = std::unordered_map<size_t, UECS::Entity>;
		struct DeserializeContext {
			const EntityIdxMap& entityIdxMap;
			const Visitor<void(void*, const rapidjson::Value&, DeserializeContext&)>& deserializer;
			const UECS::World* w{ nullptr };
		};
		using DeserializeFunc = std::function<void(void*, const rapidjson::Value&, DeserializeContext&)>;

		void RegisterDeserializeFunction(TypeID id, DeserializeFunc);

		template<typename Func> // (const T*, SerializeContext&)
		void RegisterDeserializeFunction(Func&& func);

		UDRefl::SharedObject Deserialize(std::string_view json);

		bool DeserializeToWorld(UECS::World*, std::string_view json);

		static UDRefl::SharedObject DeserializeRecursion(const rapidjson::Value& value, Serializer::DeserializeContext& ctx);

	private:
		Serializer();
		~Serializer();

		struct Impl;
		Impl* pImpl;
	};
}

#include "details/Serializer.inl"
