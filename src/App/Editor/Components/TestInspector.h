#pragma once

#include <USRefl/USRefl.h>
#include <UECS/Entity.h>
#include <UGM/UGM.h>

namespace Ubpa::DustEngine {
	enum class TestEnum {
		A,
		B,
		C
	};

	struct TestInspector {
		bool v_bool;
		uint8_t v_uint8;
		uint16_t v_uint16;
		uint32_t v_uint32;
		uint64_t v_uint64;
		int8_t v_int8;
		int16_t v_int16;
		int32_t v_int32;
		int64_t v_int64;
		//void* v_nullptr;
		float v_float;
		double v_double;
		std::string v_string;
		UECS::Entity v_entity{ UECS::Entity::Invalid() };
		//const HLSLFile* v_hlslFile;
		std::array<int, 3> v_array;
		std::array<std::array<float, 2>, 3> v_array2;
		bboxf3 v_bbox;
		vecf3 v_vec;
		rgbf v_rgb;
		rgbaf v_rgba;
		std::vector<std::string> v_vector{ "abc","edf" };
		std::deque<size_t> v_deque;
		std::forward_list<size_t> v_forward_list;
		std::list<size_t> v_list;
		//std::set<size_t> v_set;
		//std::multiset<size_t> v_multiset;
		//std::unordered_set<size_t> v_unordered_set;
		//std::unordered_multiset<size_t> v_unordered_multiset;
		std::map<std::string, std::array<int, 3>> v_map{ {"a", {1,2,3}}, {"b", {4,5,6}} };
		//std::multimap<size_t, std::string> v_multimap;
		//std::unordered_map<std::string, std::string> v_unordered_map;
		//std::unordered_multimap<std::string, std::string> v_unordered_multimap;
		std::tuple<size_t, bool, float> v_tuple;
		std::pair<size_t, bool> v_pair;
		//std::vector<Entity> v_vector_entity;
		//UserType v_usertype;
		TestEnum v_enum{ TestEnum::B };
	};
}

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::TestEnum>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::TestEnum>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"A", Ubpa::DustEngine::TestEnum::A},
		Field{"B", Ubpa::DustEngine::TestEnum::B},
		Field{"C", Ubpa::DustEngine::TestEnum::C},
	};
};

template<>
struct Ubpa::USRefl::TypeInfo<Ubpa::DustEngine::TestInspector>
	: Ubpa::USRefl::TypeInfoBase<Ubpa::DustEngine::TestInspector>
{
	static constexpr AttrList attrs = {};

	static constexpr FieldList fields = {
		Field{"v_bool", &Ubpa::DustEngine::TestInspector::v_bool},
		Field{"v_uint8", &Ubpa::DustEngine::TestInspector::v_uint8},
		Field{"v_uint16", &Ubpa::DustEngine::TestInspector::v_uint16},
		Field{"v_uint32", &Ubpa::DustEngine::TestInspector::v_uint32},
		Field{"v_uint64", &Ubpa::DustEngine::TestInspector::v_uint64},
		Field{"v_int8", &Ubpa::DustEngine::TestInspector::v_int8},
		Field{"v_int16", &Ubpa::DustEngine::TestInspector::v_int16},
		Field{"v_int32", &Ubpa::DustEngine::TestInspector::v_int32},
		Field{"v_int64", &Ubpa::DustEngine::TestInspector::v_int64},
		Field{"v_float", &Ubpa::DustEngine::TestInspector::v_float},
		Field{"v_double", &Ubpa::DustEngine::TestInspector::v_double},
		Field{"v_string", &Ubpa::DustEngine::TestInspector::v_string},
		Field{"v_entity", &Ubpa::DustEngine::TestInspector::v_entity},
		Field{"v_array", &Ubpa::DustEngine::TestInspector::v_array},
		Field{"v_array2", &Ubpa::DustEngine::TestInspector::v_array2},
		Field{"v_bbox", &Ubpa::DustEngine::TestInspector::v_bbox},
		Field{"v_vec", &Ubpa::DustEngine::TestInspector::v_vec},
		Field{"v_rgb", &Ubpa::DustEngine::TestInspector::v_rgb},
		Field{"v_rgba", &Ubpa::DustEngine::TestInspector::v_rgba},
		Field{"v_vector", &Ubpa::DustEngine::TestInspector::v_vector},
		Field{"v_deque", &Ubpa::DustEngine::TestInspector::v_deque},
		Field{"v_list", &Ubpa::DustEngine::TestInspector::v_list},
		Field{"v_forward_list", &Ubpa::DustEngine::TestInspector::v_forward_list},
		Field{"v_map", &Ubpa::DustEngine::TestInspector::v_map},
		Field{"v_tuple", &Ubpa::DustEngine::TestInspector::v_tuple},
		Field{"v_pair", &Ubpa::DustEngine::TestInspector::v_pair},
		Field{"v_enum", &Ubpa::DustEngine::TestInspector::v_enum}
	};
};


