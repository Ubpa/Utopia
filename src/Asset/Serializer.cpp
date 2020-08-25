#include <DustEngine/Asset/Serializer.h>

#include <UECS/World.h>
#include <UECS/IListener.h>

#include <UDP/Visitor/cVisitor.h>

#include <rapidjson/error/en.h>

#include <USRefl/USRefl.h>

#include <iostream>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;
using namespace Ubpa;
using namespace rapidjson;
using namespace std;

struct Serializer::Impl : IListener {
	StringBuffer sb;
	Writer<StringBuffer> writer;

	Visitor<void(const void*, Writer<StringBuffer>&)> cmptSerializer;
	std::map<CmptType, CmptDeserializeFunc> cmptDeserializer;

	Impl() {
		writer.Reset(sb);
	}

	void Clear() {
		sb.Clear();
		writer.Reset(sb);
	}

	virtual void EnterWorld(const World* world) override {
		writer.StartObject();
		writer.Key("entityMngr");
	}
	virtual void ExistWorld(const World* world) override {
		writer.EndObject();
	}

	virtual void EnterSystemMngr(const SystemMngr*) override {}
	virtual void ExistSystemMngr(const SystemMngr*) override {}

	virtual void EnterSystem(const System*) override {}
	virtual void ExistSystem(const System*) override {}

	virtual void EnterEntityMngr(const EntityMngr*) override {
		writer.StartObject();
		writer.Key("entities");
		writer.StartArray();
	}
	virtual void ExistEntityMngr(const EntityMngr*) override {
		writer.EndArray(); // entities
		writer.EndObject();
	}

	virtual void EnterEntity(const Entity* e) override {
		writer.StartObject();
		writer.Key("index");
		writer.Uint64(e->Idx());
		writer.Key("components");
		writer.StartArray();
	}
	virtual void ExistEntity(const Entity*) override {
		writer.EndArray(); // components
		writer.EndObject();
	}

	virtual void EnterCmptPtr(const CmptPtr* p) override {
		writer.StartObject();
		writer.Key("type");
		writer.Uint64(p->Type().HashCode());
		if (cmptSerializer.IsRegistered(p->Type().HashCode()))
			cmptSerializer.Visit(p->Type().HashCode(), p->Ptr(), writer);
	}
	virtual void ExistCmptPtr(const CmptPtr*) override {
		writer.EndObject();
	}
};

Serializer::Serializer()
	: pImpl{ new Impl }
{}

Serializer::~Serializer() {
	delete pImpl;
}

string Serializer::ToJSON(const World* world) {
	world->Accept(pImpl);
	auto json = pImpl->sb.GetString();
	pImpl->Clear();
	return json;
}

World* Serializer::ToWorld(string_view json) {
	auto world = new World;
	Document doc;
	ParseResult rst = doc.Parse(json.data());

	if (!rst) {
		cerr << "ERROR::DeserializerJSON::DeserializeScene:" << endl
			<< "\t" << "JSON parse error: "
			<< GetParseError_En(rst.Code()) << " (" << rst.Offset() << ")" << endl;
		return nullptr;
	}

	auto entityMngr = doc["entityMngr"].GetObject();
	auto entities = entityMngr["entities"].GetArray();

	for (const auto& val_e : entities) {
		const auto& e = val_e.GetObject();
		const auto& components = e["components"].GetArray();
		auto [entity] = world->entityMngr.Create();
		for (const auto& val_cmpt : components) {
			const auto& cmpt = val_cmpt.GetObject();
			size_t cmptID = cmpt["type"].GetUint64();
			auto type = CmptType{ cmptID };
			auto target = pImpl->cmptDeserializer.find(type);
			if (target != pImpl->cmptDeserializer.end())
				target->second(world, entity, cmpt);
			else {
				world->entityMngr.Attach(entity, &type, 1);
				// use binary to init
				// base64 string
			}
		}
	}

	return world;
}

void Serializer::RegisterComponentSerializeFunction(UECS::CmptType type, CmptSerializeFunc func) {
	pImpl->cmptSerializer.Register(type.HashCode(), std::move(func));
}

void Serializer::RegisterComponentDeserializeFunction(UECS::CmptType type, CmptDeserializeFunc func) {
	pImpl->cmptDeserializer.emplace(type, std::move(func));
}
