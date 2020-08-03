#include <DustEngine/LuaECS/LuaECS.h>

#include <iostream>
using namespace std;

int main() {
	char buff[256];
	int error;
	lua_State* L = luaL_newstate(); /* opens Lua */
	luaL_openlibs(L); /* opens the standard libraries */
	Ubpa::DustEngine::LuaECS::Instance().Init(L);
	{
		sol::state_view lua(L);
		const char code[] = R"(
rtd = RTDCmptTraits.Instance()

luaCmpt0 = CmptType.new("luaCmpt0", 0)
luaCmpt1 = CmptType.new("luaCmpt1", 0)

rtd:RegisterSize(luaCmpt0, 8)
rtd:RegisterSize(luaCmpt1, 8)

default_ctor = function () print("default_ctor") end
move_ctor = function () print("move_ctor") end
move_assignment = function () print("move_assignment") end
dtor = function () print("dtor") end

rtd:RegisterDefaultConstructor(luaCmpt0, default_ctor)
rtd:RegisterMoveConstructor(luaCmpt0, move_ctor)
rtd:RegisterDestructor(luaCmpt0, dtor)
rtd:RegisterMoveAssignment(luaCmpt0, move_assignment)

w = World.new()
em = w:GetEntityMngr()
e0 = em:Create(luaCmpt0, 1)
em:Attach(e0, luaCmpt1, 1)
e1 = em:Create(luaCmpt0, 1)
em:Attach(e1, luaCmpt1, 1)
em:Detach(e0, luaCmpt0, 1)

w:Update()
print(w:DumpUpdateJobGraph())
)";
		cout << code << endl
			<< "----------------------------" << endl;
		lua.script(code);
	}

	while (fgets(buff, sizeof(buff), stdin) != NULL) {
		error = luaL_loadstring(L, buff) || lua_pcall(L, 0, 0, 0);
		if (error) {
			fprintf(stderr, "%s\n", lua_tostring(L, -1));
			lua_pop(L, 1); /* pop error message from the stack */
		}
	}
	lua_close(L);
	return 0;
}
