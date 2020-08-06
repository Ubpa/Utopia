#include <DustEngine/ScriptSystem/LuaMngr.h>
#include <UECS/World.h>

#include <iostream>
using namespace std;

int main() {
	char buff[256];
	int error;
	Ubpa::DustEngine::LuaMngr::Instance().Init();
	auto L = Ubpa::DustEngine::LuaMngr::Instance().Request();
	{
		sol::state_view lua(L);
		lua.script_file("../assets/scripts/test_00.lua");
	}

	while (fgets(buff, sizeof(buff), stdin) != NULL) {
		error = luaL_loadstring(L, buff) || lua_pcall(L, 0, 0, 0);
		if (error) {
			fprintf(stderr, "%s\n", lua_tostring(L, -1));
			lua_pop(L, 1); /* pop error message from the stack */
		}
	}

	Ubpa::DustEngine::LuaMngr::Instance().Recycle(L);

	Ubpa::UECS::RTDCmptTraits::Instance().Clear();
	Ubpa::DustEngine::LuaMngr::Instance().Clear();

	return 0;
}
