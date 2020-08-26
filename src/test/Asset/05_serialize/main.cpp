#include <DustEngine/Asset/Serializer.h>

#include <DustEngine/Core/HLSLFile.h>

#include <UECS/World.h>

#include <iostream>

using namespace Ubpa::DustEngine;
using namespace Ubpa::UECS;
using namespace Ubpa;
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
	std::array<int, 3> v_array;
	std::array<std::array<float, 2>, 3> v_array2;
	bboxf3 v_bbox;
	vecf3 v_vec;
	std::vector<std::string> v_vector;
	std::deque<size_t> v_deque;
	std::forward_list<size_t> v_forward_list;
	std::list<size_t> v_list;
	std::set<size_t> v_set;
	std::multiset<size_t> v_multiset;
	std::unordered_set<size_t> v_unordered_set;
	std::unordered_multiset<size_t> v_unordered_multiset;
	std::map<std::string, std::vector<bool>> v_map;
	std::multimap<size_t, std::string> v_multimap;
	std::unordered_map<std::string, std::string> v_unordered_map;
	std::unordered_multimap<std::string, std::string> v_unordered_multimap;
	std::tuple<size_t, bool, float> v_tuple;
	std::pair<size_t, bool> v_pair;
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
		Field{"v_array", &A::v_array},
		Field{"v_array2", &A::v_array2},
		Field{"v_bbox", &A::v_bbox},
		Field{"v_vec", &A::v_vec},
		Field{"v_vector", &A::v_vector},
		Field{"v_deque", &A::v_deque},
		Field{"v_forward_list", &A::v_forward_list},
		Field{"v_list", &A::v_list},
		Field{"v_set", &A::v_set},
		Field{"v_multiset", &A::v_multiset},
		Field{"v_unordered_set", &A::v_unordered_set},
		Field{"v_unordered_multiset", &A::v_unordered_multiset},
		Field{"v_map", &A::v_map},
		Field{"v_multimap", &A::v_multimap},
		Field{"v_unordered_map", &A::v_unordered_map},
		Field{"v_unordered_multimap", &A::v_unordered_multimap},
		Field{"v_tuple", &A::v_tuple},
		Field{"v_pair", &A::v_pair},
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
	a->v_array = { 1,2,3 };
	a->v_array2 = { { {1,2},{3,4},{5,6} } };
	a->v_bbox = { {1,2,3},{4,5,6} };
	a->v_vec = { 1,2,3 };
	a->v_vector = { "str0","str1" };
	a->v_deque = { 1,2,3 };
	a->v_forward_list = { 1,2,3 };
	a->v_list = { 1,2,3 };
	a->v_set = { 1,2,3 };
	a->v_multiset = { 1,1,2,3 };
	a->v_unordered_set = { 1,2,3 };
	a->v_unordered_multiset = { 1,1,2,3 };
	a->v_map = { { "tf",{true, false} },{ "ft",{false, true} } };
	a->v_multimap = { { 0,"a" },{ 0,"b"}, {1,"c"} };
	a->v_unordered_map = { { "a","b" },{ "b","c"}, {"c","d"} };
	a->v_unordered_multimap = { { "a","a" },{ "a","b"}, {"b","a"} };
	a->v_tuple = { 0,false,1.5f };
	a->v_pair = { 0,false };

	w.entityMngr.Create();
	auto json = Serializer::Instance().ToJSON(&w);
	cout << json << endl;
	auto new_w = Serializer::Instance().ToWorld(json);
	auto new_json = Serializer::Instance().ToJSON(new_w);
	cout << new_json << endl;
	std::pair p{ 1,2 };
	std::apply([](auto...) {}, p);
	return 0;
}
