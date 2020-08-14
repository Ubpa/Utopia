#include <DustEngine/Asset/AssetMngr.h>
#include <DustEngine/ScriptSystem/LuaScript.h>

#include <iostream>

using namespace Ubpa::DustEngine;

int main() {
	AssetMngr::Instance().Init();
	AssetMngr::Instance().ImportAsset("../assets/scripts/test_00.lua");
	auto luaS = AssetMngr::Instance().LoadAssetAs<LuaScript>("../assets/scripts/test_00.lua");
	std::cout << luaS->GetString() << std::endl;
	AssetMngr::Instance().Clear();
}
