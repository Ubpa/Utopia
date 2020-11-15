#include <Utopia/Asset/Serializer.h>

#include <UECS/World.h>
#include <UECS/IListener.h>

#include <rapidjson/error/en.h>

#include <USRefl/USRefl.h>

#include <iostream>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;
using namespace Ubpa;
using namespace rapidjson;
using namespace std;

struct Serializer::Impl {
	Visitor<void(const void*, SerializeContext&)> serializer;
	Visitor<void(void*, const rapidjson::Value&, DeserializeContext&)> deserializer;

	struct WorldSerializer : IListener {
		SerializeContext ctx;
		const World* w;

		WorldSerializer(const Visitor<void(const void*, SerializeContext&)>& serializer)
			: ctx{ serializer }
		{
			ctx.writer.Reset(ctx.sb);
		}

		virtual void EnterWorld(const World* world) override {
			ctx.writer.StartObject();
			ctx.writer.Key(Serializer::Key::ENTITY_MNGR);
			w = world;
		}
		virtual void ExistWorld(const World* world) override {
			ctx.writer.EndObject();
			w = nullptr;
		}

		virtual void EnterEntityMngr(const EntityMngr*) override {
			ctx.writer.StartObject();
			ctx.writer.Key(Serializer::Key::ENTITIES);
			ctx.writer.StartArray();
		}
		virtual void ExistEntityMngr(const EntityMngr*) override {
			ctx.writer.EndArray(); // entities
			ctx.writer.EndObject();
		}

		virtual void EnterEntity(Entity e) override {
			ctx.writer.StartObject();
			ctx.writer.Key(Key::INDEX);
			ctx.writer.Uint64(e.Idx());
			ctx.writer.Key(Key::COMPONENTS);
			ctx.writer.StartArray();
		}
		virtual void ExistEntity(Entity) override {
			ctx.writer.EndArray(); // components
			ctx.writer.EndObject();
		}

		virtual void EnterCmpt(CmptPtr p) override {
			ctx.writer.StartObject();
			ctx.writer.Key(Key::TYPE);
			ctx.writer.Uint64(p.Type().HashCode());
			auto name = w->entityMngr.cmptTraits.Nameof(p.Type());
			if (!name.empty()) {
				ctx.writer.Key(Key::NAME);
				ctx.writer.String(name.data());
			}
			ctx.writer.Key(Key::CONTENT);
			if (ctx.serializer.IsRegistered(p.Type().HashCode()))
				ctx.serializer.Visit(p.Type().HashCode(), p.Ptr(), ctx);
			else
				ctx.writer.Key(Key::NOT_SUPPORT);
		}
		virtual void ExistCmpt(CmptPtr) override {
			ctx.writer.EndObject();
		}
	};
};

Serializer::Serializer()
	: pImpl{ new Impl }
{}

Serializer::~Serializer() {
	delete pImpl;
}

string Serializer::ToJSON(const World* world) {
	Impl::WorldSerializer worldSerializer(pImpl->serializer);
	world->Accept(&worldSerializer);
	auto json = worldSerializer.ctx.sb.GetString();
	return json;
}

string Serializer::ToJSON(size_t ID, const void* obj) {
	SerializeContext ctx{ pImpl->serializer };
	pImpl->serializer.Visit(ID, obj, ctx);
	auto json = ctx.sb.GetString();
	return json;
}

bool Serializer::ToWorld(UECS::World* world, string_view json) {
	Document doc;
	ParseResult rst = doc.Parse(json.data());

	if (!rst) {
		cerr << "ERROR::Serializer::ToWorld:" << endl
			<< "\t" << "JSON parse error: "
			<< GetParseError_En(rst.Code()) << " (" << rst.Offset() << ")" << endl;
		return false;
	}

	auto entityMngr = doc[Serializer::Key::ENTITY_MNGR].GetObject();
	auto entities = entityMngr[Serializer::Key::ENTITIES].GetArray();

	// 1. use free entry
	// 2. use new entry
	EntityIdxMap entityIdxMap;
	entityIdxMap.reserve(entities.Size());

	const auto& freeEntries = world->entityMngr.GetEntityFreeEntries();
	size_t leftFreeEntryNum = freeEntries.size();
	size_t newEntityIndex = world->entityMngr.TotalEntityNum() + leftFreeEntryNum;
	for (const auto& val_e : entities) {
		const auto& e = val_e.GetObject();
		size_t index = e[Key::INDEX].GetUint64();
		if (leftFreeEntryNum > 0) {
			size_t freeIdx = freeEntries[--leftFreeEntryNum];
			size_t version = world->entityMngr.GetEntityVersion(freeIdx);
			entityIdxMap.emplace(index, Entity{ freeIdx, version });
		}
		else
			entityIdxMap.emplace(index, Entity{ newEntityIndex++, 0 });
	}

	DeserializeContext ctx{ entityIdxMap, pImpl->deserializer };

	for (const auto& val_e : entities) {
		const auto& jsonEntity = val_e.GetObject();
		const auto& jsonCmpts = jsonEntity[Key::COMPONENTS].GetArray();

		std::vector<CmptType> cmptTypes;
		cmptTypes.resize(jsonCmpts.Size());
		for (SizeType i = 0; i < jsonCmpts.Size(); i++) {
			const auto& cmpt = jsonCmpts[i].GetObject();
			size_t cmptID = cmpt[Key::TYPE].GetUint64();
			cmptTypes[i] = CmptType{ cmptID };
		}

		auto entity = world->entityMngr.Create(Span{ cmptTypes.data(), cmptTypes.size() });
		for (size_t i = 0; i < cmptTypes.size(); i++) {
			if (pImpl->deserializer.IsRegistered(cmptTypes[i].HashCode())) {
				pImpl->deserializer.Visit(
					cmptTypes[i].HashCode(),
					world->entityMngr.Get(entity, cmptTypes[i]).Ptr(),
					jsonCmpts[static_cast<SizeType>(i)].GetObject()[Key::CONTENT],
					ctx
				);
			}
		}
	}

	return true;
}

bool Serializer::ToUserType(std::string_view json, size_t ID, void* obj) {
	Document doc;
	ParseResult rst = doc.Parse(json.data());
	EntityIdxMap emptyMap;
	DeserializeContext ctx{ emptyMap, pImpl->deserializer };
	pImpl->deserializer.Visit(ID, obj, doc, ctx);
	return true;
}

void Serializer::RegisterSerializeFunction(size_t id, SerializeFunc func) {
	pImpl->serializer.Register(id, std::move(func));
}

void Serializer::RegisterDeserializeFunction(size_t id, DeserializeFunc func) {
	pImpl->deserializer.Register(id, std::move(func));
}

void Serializer::RegisterComponentSerializeFunction(UECS::CmptType type, SerializeFunc func) {
	RegisterSerializeFunction(type.HashCode(), std::move(func));
}

void Serializer::RegisterComponentDeserializeFunction(UECS::CmptType type, DeserializeFunc func) {
	RegisterDeserializeFunction(type.HashCode(), std::move(func));
}

void Serializer::RegisterUserTypeSerializeFunction(size_t id, SerializeFunc func) {
	RegisterSerializeFunction(id, std::move(func));
}

void Serializer::RegisterUserTypeDeserializeFunction(size_t id, DeserializeFunc func) {
	RegisterDeserializeFunction(id, std::move(func));
}