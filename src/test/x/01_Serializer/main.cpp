#include <Utopia/Core/Serializer.h>
#include <Utopia/Core/AssetMngr.h>
#include <_deps/crossguid/guid.hpp>
#include <Utopia/Core/SharedVar.h>
#include <UGM/UGM.hpp>

#include <iostream>
#include <assert.h>

using namespace Ubpa;
using namespace Ubpa::Utopia;


struct A {
	bool v_b;
	std::uint8_t v_u8;
	std::uint16_t v_u16;
	std::uint32_t v_u32;
	std::uint64_t v_u64;
	std::int8_t v_8;
	std::int16_t v_16;
	std::int32_t v_32;
	std::int64_t v_64;
	float v_f32;
	double v_f64;
	constexpr auto operator<=>(const A&)const = default;
};

struct B {
	std::string v_str;
	std::pmr::string v_pmr_str;
	xg::Guid v_guid;
	constexpr auto operator<=>(const B&)const = default;
};

struct C {
	UDRefl::SharedObject asset;
	friend bool operator==(const C& lhs, const C& rhs) {
		return lhs.asset.GetPtr() == rhs.asset.GetPtr();
	}
};

struct D {
	std::vector<C> v_vec;
	std::vector<UDRefl::SharedObject> v_vec_obj;
	std::tuple<C, C> v_t;
	std::variant<C, int> v_v0;
	std::variant<C, int> v_v1;
	std::optional<C> v_o0;
	std::optional<C> v_o1;
	vecf3 v_vecf3;
	friend bool operator==(const D& lhs, const D& rhs) {
		return lhs.v_vec == rhs.v_vec
			&& lhs.v_vec_obj == rhs.v_vec_obj
			&& lhs.v_t == rhs.v_t
			&& lhs.v_v0 == rhs.v_v0
			&& lhs.v_v1 == rhs.v_v1
			&& lhs.v_o0 == rhs.v_o0
			&& lhs.v_o1 == rhs.v_o1
			&& lhs.v_vecf3 == rhs.v_vecf3;
	}
};

struct E {
	SharedVar<DefaultAsset> asset;
	friend bool operator==(const E& lhs, const E& rhs) {
		return lhs.asset.get() == rhs.asset.get();
	}
};

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	{
		UDRefl::Mngr.RegisterType<A>();
		UDRefl::Mngr.SimpleAddField<&A::v_b>("v_b");
		UDRefl::Mngr.SimpleAddField<&A::v_u8>("v_u8");
		UDRefl::Mngr.SimpleAddField<&A::v_u16>("v_u16");
		UDRefl::Mngr.SimpleAddField<&A::v_u32>("v_u32");
		UDRefl::Mngr.SimpleAddField<&A::v_u64>("v_u64");
		UDRefl::Mngr.SimpleAddField<&A::v_8>("v_8");
		UDRefl::Mngr.SimpleAddField<&A::v_16>("v_16");
		UDRefl::Mngr.SimpleAddField<&A::v_32>("v_32");
		UDRefl::Mngr.SimpleAddField<&A::v_64>("v_64");
		UDRefl::Mngr.SimpleAddField<&A::v_f32>("v_f32");
		UDRefl::Mngr.SimpleAddField<&A::v_f64>("v_f64");
		A a{
			.v_b = true,
			.v_u8 = 32,
			.v_u16 = 64,
			.v_u32 = 128,
			.v_u64 = 12,
			.v_8 = 1,
			.v_16 = 3,
			.v_32 = 5,
			.v_64 = 9,
			.v_f32 = 1.23f,
			.v_f64 = 3.21,
		};
		std::string json = Serializer::Instance().Serialize(&a);
		std::cout << json << std::endl;
		A a2 = Serializer::Instance().Deserialize(json).As<A>();
		assert(a2 == a);
	}
	{
		UDRefl::Mngr.RegisterType<B>();
		UDRefl::Mngr.AddField<&B::v_str>("v_str");
		UDRefl::Mngr.AddField<&B::v_pmr_str>("v_pmr_str");
		UDRefl::Mngr.AddField<&B::v_guid>("v_guid");
		B b{
			.v_str = "abc",
			.v_pmr_str = "def",
			.v_guid = xg::newGuid()
		};
		std::string json = Serializer::Instance().Serialize(&b);
		std::cout << json << std::endl;
		B b2 = Serializer::Instance().Deserialize(json).As<B>();
		assert(b2 == b);
	}

	std::filesystem::path root = LR"(..\src\test\x\01_Serializer)";
	AssetMngr::Instance().SetRootPath(root);
	auto asset = AssetMngr::Instance().LoadMainAsset(LR"(main.cpp)");

	{
		UDRefl::Mngr.RegisterType<C>();

		UDRefl::Mngr.AddField<&C::asset>("asset");
		C c{
			.asset = asset
		};
		std::string json = Serializer::Instance().Serialize(&c);
		std::cout << json << std::endl;
		C c2 = Serializer::Instance().Deserialize(json).As<C>();
		assert(c2 == c);
	}
	{
		UDRefl::Mngr.RegisterType<D>();
		UDRefl::Mngr.AddField<&D::v_vec>("v_vec");
		UDRefl::Mngr.AddField<&D::v_vec_obj>("v_vec_obj");
		UDRefl::Mngr.AddField<&D::v_t>("v_t");
		UDRefl::Mngr.AddField<&D::v_v0>("v_v0");
		UDRefl::Mngr.AddField<&D::v_v1>("v_v1");
		UDRefl::Mngr.AddField<&D::v_o0>("v_o0");
		UDRefl::Mngr.AddField<&D::v_o1>("v_o1");
		UDRefl::Mngr.AddField<&D::v_vecf3>("v_vecf3");
		D d{
			.v_vec = {C{asset},C{asset}},
			.v_t = {C{asset},C{asset}},
			.v_v0 = {C{asset}},
			.v_v1 = {1},
			.v_o0 = {C{asset}},
			.v_vecf3 = {3.21f,1.24f,6.1f}
		};
		std::string json = Serializer::Instance().Serialize(&d);
		std::cout << json << std::endl;
		D d2 = Serializer::Instance().Deserialize(json).As<D>();
		assert(d2 == d);
	}
	{
		UDRefl::Mngr.RegisterType<E>();

		UDRefl::Mngr.AddField<&E::asset>("asset");
		E e{
			.asset = asset
		};
		std::string json = Serializer::Instance().Serialize(&e);
		std::cout << json << std::endl;
		E e2 = Serializer::Instance().Deserialize(json).As<E>();
		assert(e2 == e);
	}
}
