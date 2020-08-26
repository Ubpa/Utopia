#pragma once

#include <UECS/World.h>

#include <UDP/Visitor/cVisitor.h>
#include <UDP/Visitor/ncVisitor.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <string>

namespace Ubpa::DustEngine {
	class Serializer {
	public:
		static Serializer& Instance() {
			static Serializer instance;
			return instance;
		}

		using JSONWriter = rapidjson::Writer<rapidjson::StringBuffer>;
		struct SerializeContext {
			JSONWriter* const writer;
			const Visitor<void(const void*, SerializeContext)>* const fieldSerializer;
		};
		using SerializeFunc = std::function<void(const void*, SerializeContext)>;
		using EntityIdxMap = std::unordered_map<size_t, UECS::Entity>;
		struct DeserializeContext {
			const EntityIdxMap* entityIdxMap;
			const Visitor<void(void*, const rapidjson::Value&, DeserializeContext)>*const fieldDeserializer;
		};
		using DeserializeFunc = std::function<void(void*, const rapidjson::Value&, DeserializeContext)>;

		void RegisterComponentSerializeFunction(UECS::CmptType, SerializeFunc);
		void RegisterComponentDeserializeFunction(UECS::CmptType, DeserializeFunc);

		template<typename Func>
		void RegisterComponentSerializeFunction(Func&& func);
		template<typename Cmpt>
		void RegisterComponentSerializeFunction();

		template<typename Func>
		void RegisterComponentDeserializeFunction(Func&& func);
		template<typename Cmpt>
		void RegisterComponentDeserializeFunction();

		template<typename Func>
		void RegisterUserTypeSerializeFunction(Func&& func);
		template<typename Func>
		void RegisterUserTypeDeserializeFunction(Func&& func);

		std::string ToJSON(const UECS::World*);
		void ToWorld(UECS::World*, std::string_view json);
	private:
		void RegisterUserTypeSerializeFunction(size_t id, SerializeFunc func);
		void RegisterUserTypeDeserializeFunction(size_t id, DeserializeFunc func);

		Serializer();
		~Serializer();

		struct Impl;
		Impl* pImpl;
	};
}

#include "details/Serializer.inl"
