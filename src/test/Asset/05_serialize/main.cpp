#include <DustEngine/Asset/Serializer.h>

#include <DustEngine/Core/HLSLFile.h>

#include <UECS/World.h>

#include <iostream>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;
using namespace std;

struct A {
	bool v_bool;
	uint8_t v_uint8;
	uint16_t v_uint16;
	uint32_t v_uint32;
	uint64_t v_uint64;
	int8_t v_int8;
	int16_t v_int16;
	int32_t v_int32;
	int64_t v_int64;
	void* v_nullptr;
	float v_float;
	double v_double;
	std::string v_string;
	Entity v_entity{ Entity::Invalid() };
	const HLSLFile* v_hlslFile;
};

template<>
struct Ubpa::USRefl::TypeInfo<A>
	: Ubpa::USRefl::TypeInfoBase<A>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"v_bool", &A::v_bool},
		Field{"v_uint8", &A::v_uint8},
		Field{"v_uint16", &A::v_uint16},
		Field{"v_uint32", &A::v_uint32},
		Field{"v_uint64", &A::v_uint64},
		Field{"v_int8", &A::v_int8},
		Field{"v_int16", &A::v_int16},
		Field{"v_int32", &A::v_int32},
		Field{"v_int64", &A::v_int64},
		Field{"v_nullptr", &A::v_nullptr},
		Field{"v_float", &A::v_float},
		Field{"v_double", &A::v_double},
		Field{"v_string", &A::v_string},
		Field{"v_entity", &A::v_entity},
		Field{"v_hlslFile", &A::v_hlslFile},
	};
};

int main() {
	/*Serializer::Instance().RegisterComponentSerializeFunction([](const A* a, Serializer::JSONWriter& writer) {
		writer.Key("data");
		writer.Double(a->data);
	});*/
	Serializer::Instance().RegisterComponentSerializeFunction<A>();
	Serializer::Instance().RegisterComponentDeserializeFunction<A>();

	World w;

	auto [e, a] = w.entityMngr.Create<A>();
	a->v_bool = true;
	a->v_uint8 = { 8 };
	a->v_uint16 = { 16 };
	a->v_uint32 = { 32 };
	a->v_uint64 = { 64 };
	a->v_int8 = { -8 };
	a->v_int16 = { -16 };
	a->v_int32 = { -32 };
	a->v_int64 = { -64 };
	a->v_nullptr = { nullptr };
	a->v_float = { 0.1f };
	a->v_double = { 0.2 };
	a->v_string = { "hello world" };
	//a->v_entity = { Entity::Invalid() };
	a->v_hlslFile = AssetMngr::Instance().LoadAsset<HLSLFile>("../assets/shaders/Default.hlsl");

	w.entityMngr.Create();

	auto json = Serializer::Instance().ToJSON(&w);
	cout << json << endl;
	auto new_w = Serializer::Instance().ToWorld(json);
	auto new_json = Serializer::Instance().ToJSON(new_w);
	cout << new_json << endl;

	return 0;
}
