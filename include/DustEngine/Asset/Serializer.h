#pragma once

#include <UECS/World.h>

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
		using CmptSerializeFunc = std::function<void(const void*, JSONWriter&)>;
		using JSONCmpt = rapidjson::GenericObject<true, rapidjson::Value>;
		using CmptDeserializeFunc = std::function<void(UECS::World*, UECS::Entity, const JSONCmpt&)>;

		void RegisterComponentSerializeFunction(UECS::CmptType, CmptSerializeFunc);
		void RegisterComponentDeserializeFunction(UECS::CmptType, CmptDeserializeFunc);
		template<typename Func>
		void RegisterComponentSerializeFunction(Func&& func);
		template<typename Cmpt>
		void RegisterComponentSerializeFunction();
		template<typename Cmpt>
		void RegisterComponentDeserializeFunction();

		std::string ToJSON(const UECS::World*);
		UECS::World* ToWorld(std::string_view json);
	private:
		Serializer();
		~Serializer();

		struct Impl;
		Impl* pImpl;
	};
}

#include "details/Serializer.inl"
