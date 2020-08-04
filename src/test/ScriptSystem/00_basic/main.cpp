#include <DustEngine/ScriptSystem/LuaMngr.h>

#include <iostream>
using namespace std;

int main() {
	char buff[256];
	int error;
	Ubpa::DustEngine::LuaMngr::Instance().Init();
	auto L = Ubpa::DustEngine::LuaMngr::Instance().Request();
	{
		sol::state_view lua(L);
		const char code[] = R"(
rtd = RTDCmptTraits.Instance()

luaCmptType0 = CmptType.new("Cmpt0", AccessMode.WRITE)
luaCmptType1 = CmptType.new("Cmpt1", AccessMode.WRITE)

-- Cmpt0 : 64 bytes
--  0 - 16 table
-- 16 - 24 double
-- 24 - 28 int32
-- 28 - 32 int32
-- 32 - 64 str[32]

rtd:RegisterSize(luaCmptType0, 64)

default_ctor = function (ptr)
  print("default_ctor")
  local cmpt = LuaCmpt.new(luaCmptType0, ptr)
  cmpt:SetZero()
  cmpt:DefaultConstructTable(0)
end

copy_ctor = function (dst, src)
  print("copy ctor")
  local cmpt_dst = LuaCmpt.new(luaCmptType0, dst)
  local cmpt_src = LuaCmpt.new(luaCmptType0, src)
  cmpt_dst:MemCpy(src)
  cmpt_dst:CopyConstructTable(cmpt_src, 0)
end

move_ctor = function (dst, src)
  print("move_ctor")
  local cmpt_dst = LuaCmpt.new(luaCmptType0, dst)
  local cmpt_src = LuaCmpt.new(luaCmptType0, src)
  cmpt_dst:MemCpy(src)
  cmpt_dst:MoveConstructTable(cmpt_src, 0)
end

move_assignment = function (dst, src)
  print("move_assignment")
  local cmpt_dst = LuaCmpt.new(luaCmptType0, dst)
  local cmpt_src = LuaCmpt.new(luaCmptType0, src)
  --cmpt_dst:MemCpy(src)
  cmpt_dst:MemCpy(src, 16, 24)
  cmpt_dst:MoveAssignTable(cmpt_src, 0)
end

dtor = function (ptr)
  print("dtor")
  local cmpt = LuaCmpt.new(luaCmptType0, ptr)
  cmpt:DestructTable(0)
end

rtd:RegisterSize(luaCmptType1, 8)

rtd:RegisterDefaultConstructor(luaCmptType0, default_ctor)
rtd:RegisterCopyConstructor(luaCmptType0, copy_ctor)
rtd:RegisterMoveConstructor(luaCmptType0, move_ctor)
rtd:RegisterMoveAssignment(luaCmptType0, move_assignment)
rtd:RegisterDestructor(luaCmptType0, dtor)

w = World.new()
em = w:GetEntityMngr()
cmpts = LuaArray_CmptType.new()
cmpts:PushBack(luaCmptType0)
cmpts:PushBack(luaCmptType1)

e0 = em:Create(cmpts:Data(), cmpts:Size())
e1 = em:Create(cmpts:Data(), cmpts:Size())

em:Detach(e0, luaCmptType1, 1)

e0_cmpt0 = em:Get(e0, luaCmptType0)
luaCmpt0 = LuaCmpt.new(e0_cmpt0)
--t = luaCmpt:GetTable(0)
--print(luaCmpt:GetDouble(0))

--w:Update()
--print(w:DumpUpdateJobGraph())
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

	Ubpa::DustEngine::LuaMngr::Instance().Recycle(L);
	Ubpa::DustEngine::LuaMngr::Instance().Clear();

	return 0;
}
