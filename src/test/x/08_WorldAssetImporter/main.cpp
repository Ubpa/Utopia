#include <Utopia/Core/Serializer.h>
#include <Utopia/Core/AssetMngr.h>
#include <Utopia/Core/Register_Core.h>
#include <_deps/crossguid/guid.hpp>
#include <Utopia/Core/SharedVar.h>
#include <Utopia/Core/Components/Components.h>
#include <Utopia/Core/WorldAssetImporter.h>
#include <UECS/UECS.hpp>
#include <UGM/UGM.hpp>

#include <iostream>
#include <assert.h>

using namespace Ubpa;
using namespace Ubpa::Utopia;
using namespace Ubpa::UECS;

int main() {
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	const std::filesystem::path root = LR"(..\src\test\x\08_WorldAssetImporter\assets)";
	std::filesystem::create_directory(root);
	AssetMngr::Instance().SetRootPath(root);
	AssetMngr::Instance().RegisterAssetImporterCreator(std::make_shared<WorldAssetImporterCreator>());

	UDRefl_Register_Core();
	
	{ // empty world
		const World w;
		const std::filesystem::path p = LR"(empty.world)";
		bool success = AssetMngr::Instance().CreateAsset(std::make_shared<WorldAsset>(&w), p);
		assert(success);
		assert(std::filesystem::exists(AssetMngr::Instance().GetFullPath(p)));
		assert(std::filesystem::exists(AssetMngr::Instance().GetFullPath(p).concat(LR"(.meta)")));
		AssetMngr::Instance().UnloadAsset(p);
		auto wa = AssetMngr::Instance().LoadAsset<WorldAsset>(p);
		assert(wa.get());
		assert(wa->GetData() == R"({"__EntityMngr":{"__Entities":[]}})");
		World w2;
		bool tsuccess = wa->ToWorld(&w2);
		assert(tsuccess);
	}
	{ // world -> empty world
		World w;
		World_Register_Core(&w);

		{
			auto e = w.entityMngr.Create(TypeIDs_of<Translation, Scale, NonUniformScale, Rotation, RotationEuler>);
			w.entityMngr.WriteComponent<Translation>(e)->value = { 3.f,2.f,1.f };
			w.entityMngr.WriteComponent<Scale>(e)->value = 3.5f;
			w.entityMngr.WriteComponent<NonUniformScale>(e)->value = { 4.f,5.f,6.f };
		}
		w.entityMngr.Create(TypeIDs_of<Roamer>);
		w.entityMngr.Create(TypeIDs_of<Input>);
		{
			auto e = w.entityMngr.Create(TypeIDs_of<WorldTime, Utopia::Name>);
			w.entityMngr.WriteComponent<Utopia::Name>(e)->value = "hello";
		}
		std::string str = Serializer::Instance().Serialize(&w);
		std::cout << str << std::endl;

		const std::filesystem::path p = LR"(w.world)";
		bool success = AssetMngr::Instance().CreateAsset(std::make_shared<WorldAsset>(str), p);
		assert(success);
		assert(std::filesystem::exists(AssetMngr::Instance().GetFullPath(p)));
		assert(std::filesystem::exists(AssetMngr::Instance().GetFullPath(p).concat(LR"(.meta)")));
		AssetMngr::Instance().UnloadAsset(p);
		auto wa = AssetMngr::Instance().LoadAsset<WorldAsset>(p);
		assert(wa.get());
		assert(wa->GetData() == str);

		World w2;
		World_Register_Core(&w2);
		auto dsuccess = Serializer::Instance().DeserializeToWorld(&w2, wa->GetData());
		assert(dsuccess);
		assert(w.entityMngr.ReadSingleton<Translation>()->value == w2.entityMngr.ReadSingleton<Translation>()->value);
		assert(w.entityMngr.ReadSingleton<Scale>()->value == w2.entityMngr.ReadSingleton<Scale>()->value);
		assert(w.entityMngr.ReadSingleton<NonUniformScale>()->value == w2.entityMngr.ReadSingleton<NonUniformScale>()->value);
		assert(w.entityMngr.ReadSingleton<Utopia::Name>()->value == w2.entityMngr.ReadSingleton<Utopia::Name>()->value);
	}

	AssetMngr::Instance().Clear();
	std::filesystem::remove_all(root);
	return 0;
}
