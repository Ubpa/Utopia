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
		const char code[] = R"(
rtd = RTDCmptTraits.Instance()

luaCmptType0 = CmptType.new("Cmpt0", AccessMode.WRITE)
luaCmptType1 = CmptType.new("Cmpt1", AccessMode.WRITE)

-- Cmpt0 : 64 bytes
--  0 - 16 str buffer -> (init) 128 bytes string
-- 16 - 24 double {0}
-- 24 - 28 int32  {0}
-- 28 - 32 int32  {0}
-- 32 - 64 str[32] ""

rtd:RegisterSize(luaCmptType0, 64)

default_ctor = function (ptr)
  print("default ctor")
  local cmpt = LuaBuffer.new(ptr, 64)
  LuaMemory.Set(ptr, 0, 64)
  local str = LuaMemory.Malloc(128)
  local strbuf = LuaBuffer.new(str, 128)
  strbuf:SetCString(0, "hello world!")
  cmpt:SetBuffer(0, strbuf)
end

copy_ctor = function (dst, src)
  print("copy ctor")
  local cmpt_dst = LuaBuffer.new(dst, 64)
  local cmpt_src = LuaBuffer.new(src, 64)
  LuaMemory.Copy(dst, src, 64)

  local strbuf_src = cmpt_src:GetBuffer(0)
  local str_dst = LuaMemory.Malloc(strbuf_src.size)
  local strbuf_dst  LuaBuffer.new(str_dst, strbuf_src.size)
  LuaMemory.Copy(str_dst, strbuf_src.ptr, strbuf_src.size)

  cmpt_dst:SetBuffer(0, strbuf_dst)
end

move_ctor = function (dst, src)
  print("move ctor")
  local cmpt_dst = LuaBuffer.new(dst, 64)
  local cmpt_src = LuaBuffer.new(src, 64)
  LuaMemory.Copy(dst, src, 64)

  local empty_buffer = LuaBuffer.new()
  cmpt_src:SetBuffer(0, empty_buffer)
end

move_assignment = function (dst, src)
  print("move assignment")
  local cmpt_dst = LuaBuffer.new(dst, 64)
  local cmpt_src = LuaBuffer.new(src, 64)
  local strbuf_dst = cmpt_dst:GetBuffer(0)
  LuaMemory.Free(strbuf_dst.ptr)
  LuaMemory.Copy(dst, src, 64)
  local empty_buffer = LuaBuffer.new()
  cmpt_src:SetBuffer(0, empty_buffer)
end

dtor = function (ptr)
  print("dtor")
  local cmpt = LuaBuffer.new(ptr, 64)
  local strbuf = cmpt:GetBuffer(0)
  LuaMemory.Free(strbuf.ptr)
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
e0_cmpt0_buf = LuaBuffer.new(e0_cmpt0:Ptr(), 64)
e0_cmpt0_buf_strbuf = e0_cmpt0_buf:GetBuffer(0)
str = e0_cmpt0_buf_strbuf:GetCString(0)
print(str)

f = function(schedule)
  local g = function(chunk) print("entity number:", chunk:EntityNum()) end
  local luaCmptType0 = CmptType.new("Cmpt0", AccessMode.WRITE)
  local filter = EntityFilter.new()
  filter:InsertAll(luaCmptType0, 1)
  LuaSystem.RegisterChunkFunc(schedule, g, "test", filter)
end
LuaSystem.Register(w, "LuaSystem-001", f)

)";
		cout << code << endl
			<< "----------------------------" << endl;
		lua.script(code);
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

	Ubpa::UECS::RTDCmptTraits::Instance().Clear();
	Ubpa::DustEngine::LuaMngr::Instance().Clear();

	return 0;
}
