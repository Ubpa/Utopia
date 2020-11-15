#include <Utopia/Asset/Serializer.h>

#include <Utopia/Render/HLSLFile.h>

#include <UECS/World.h>

#include <iostream>

using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;
using namespace Ubpa;
using namespace std;

struct UserType0 {
	float data;
};

struct UserType1 {
	UserType0 usertype0;
};

enum class Color {
	RED,
	GREEN,
	BLUE
};

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
	std::shared_ptr<HLSLFile> v_hlslFile;
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
	std::vector<Entity> v_vector_entity;
	UserType0 v_usertype0;
	UserType1 v_usertype1;
	std::variant<std::string, size_t> v_variant0;
	std::variant<std::string, size_t> v_variant1;
	Color v_enum;
};

template<>
struct Ubpa::USRefl::TypeInfo<UserType1> :
	TypeInfoBase<UserType1>
{
#ifdef UBPA_USREFL_NOT_USE_NAMEOF
	static constexpr char name[10] = "UserType1";
#endif
	static constexpr AttrList attrs = {};
	static constexpr FieldList fields = {
		Field {TSTR("usertype0"), &Type::usertype0},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<A> :
	TypeInfoBase<A>
{
#ifdef UBPA_USREFL_NOT_USE_NAMEOF
	static constexpr char name[2] = "A";
#endif
	static constexpr AttrList attrs = {};
	static constexpr FieldList fields = {
		Field {TSTR("v_bool"), &Type::v_bool},
		Field {TSTR("v_uint8"), &Type::v_uint8},
		Field {TSTR("v_uint16"), &Type::v_uint16},
		Field {TSTR("v_uint32"), &Type::v_uint32},
		Field {TSTR("v_uint64"), &Type::v_uint64},
		Field {TSTR("v_int8"), &Type::v_int8},
		Field {TSTR("v_int16"), &Type::v_int16},
		Field {TSTR("v_int32"), &Type::v_int32},
		Field {TSTR("v_int64"), &Type::v_int64},
		Field {TSTR("v_nullptr"), &Type::v_nullptr},
		Field {TSTR("v_float"), &Type::v_float},
		Field {TSTR("v_double"), &Type::v_double},
		Field {TSTR("v_string"), &Type::v_string},
		Field {TSTR("v_entity"), &Type::v_entity, AttrList {
			Attr {TSTR(UMeta::initializer), []()->Entity { return { Entity::Invalid() }; }},
		}},
		Field {TSTR("v_hlslFile"), &Type::v_hlslFile},
		Field {TSTR("v_array"), &Type::v_array},
		Field {TSTR("v_array2"), &Type::v_array2},
		Field {TSTR("v_bbox"), &Type::v_bbox},
		Field {TSTR("v_vec"), &Type::v_vec},
		Field {TSTR("v_vector"), &Type::v_vector},
		Field {TSTR("v_deque"), &Type::v_deque},
		Field {TSTR("v_forward_list"), &Type::v_forward_list},
		Field {TSTR("v_list"), &Type::v_list},
		Field {TSTR("v_set"), &Type::v_set},
		Field {TSTR("v_multiset"), &Type::v_multiset},
		Field {TSTR("v_unordered_set"), &Type::v_unordered_set},
		Field {TSTR("v_unordered_multiset"), &Type::v_unordered_multiset},
		Field {TSTR("v_map"), &Type::v_map},
		Field {TSTR("v_multimap"), &Type::v_multimap},
		Field {TSTR("v_unordered_map"), &Type::v_unordered_map},
		Field {TSTR("v_unordered_multimap"), &Type::v_unordered_multimap},
		Field {TSTR("v_tuple"), &Type::v_tuple},
		Field {TSTR("v_pair"), &Type::v_pair},
		Field {TSTR("v_vector_entity"), &Type::v_vector_entity},
		Field {TSTR("v_usertype0"), &Type::v_usertype0},
		Field {TSTR("v_usertype1"), &Type::v_usertype1},
		Field {TSTR("v_variant0"), &Type::v_variant0},
		Field {TSTR("v_variant1"), &Type::v_variant1},
		Field {TSTR("v_enum"), &Type::v_enum},
	};
};


int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Serializer::Instance().RegisterComponentSerializeFunction<A>();
	Serializer::Instance().RegisterComponentDeserializeFunction<A>();
	Serializer::Instance().RegisterUserTypeSerializeFunction([](const UserType0* t, Serializer::SerializeContext& ctx) {
		ctx.writer.Double(static_cast<double>(t->data));
	});
	Serializer::Instance().RegisterUserTypeDeserializeFunction(
		[](UserType0* t, const rapidjson::Value& jsonValue, Serializer::DeserializeContext& ctx) {
			t->data = static_cast<float>(jsonValue.GetDouble());
		}
	);

	World w;

	auto [e0] = w.entityMngr.Create();

	auto [e1, a] = w.entityMngr.Create<A>();
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
	a->v_entity = e0;
	a->v_hlslFile = AssetMngr::Instance().LoadAsset<HLSLFile>("../assets/shaders/Geometry.hlsl");
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
	a->v_variant0 = "string";
	a->v_variant1 = static_cast<size_t>(1);
	a->v_enum = Color::GREEN;

	auto [e2] = w.entityMngr.Create();
	auto [e3] = w.entityMngr.Create();

	a->v_vector_entity = { e0,e1,e2,e3 };

	auto json = Serializer::Instance().ToJSON(&w);
	cout << json << endl;
	auto new_w = std::make_unique<World>();
	auto [new_e0] = new_w->entityMngr.Create();
	auto [new_e1] = new_w->entityMngr.Create();
	new_w->entityMngr.Destroy(new_e0);
	new_w->entityMngr.cmptTraits.Register<A>();
	Serializer::Instance().ToWorld(new_w.get(), json);
	new_w->entityMngr.Destroy(new_e1);
	auto new_json = Serializer::Instance().ToJSON(new_w.get());
	cout << new_json << endl;
	std::pair p{ 1,2 };
	std::apply([](auto...) {}, p);
	return 0;
}
