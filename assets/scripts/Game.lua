move = function(schedule)
  local sys = function(w, singletons, e, idx, cmpts)
    local type0 = CmptAccessType.new(
	  "Ubpa::DustEngine::Translation",
	  AccessMode.WRITE
	)
	
	local type1 = CmptAccessType.new(
	  "Ubpa::DustEngine::WorldTime",
	  AccessMode.LATEST_SINGLETON
	)
	
    local cmptPtr      = cmpts:GetCmpt(type0)
    local singletonPtr = singletons:GetSingleton(type1)
	
	local translate = Translation.voidp(cmptPtr:Ptr())
    local worldtime = WorldTime.voidp(singletonPtr:Ptr())
	
	-- 1 x, 2 y, 3 z
	translate.value[1] = translate.value[1] + worldtime.deltaTime * math.cos(worldtime.elapsedTime)
	translate.value[2] = translate.value[2] - worldtime.deltaTime * math.sin(worldtime.elapsedTime)
  end
  
  local filter = ArchetypeFilter.new();
  filter.all:add(CmptAccessType.new("Ubpa::DustEngine::MeshFilter", AccessMode.LATEST))
  filter.none:add(CmptType.new("Ubpa::DustEngine::Children"))
  local cLocator = CmptLocator.new(CmptAccessType.new("Ubpa::DustEngine::Translation", AccessMode.WRITE), 1)
  local sLocator = SingletonLocator.new(CmptAccessType.new("Ubpa::DustEngine::WorldTime", AccessMode.LATEST_SINGLETON), 1)
  LuaSystem.RegisterEntityJob(
    schedule,
	sys,
	"move",
	filter,
	cLocator,
	sLocator,
	true
  )
end
LuaSystem.RegisterSystem(world, "move", move)
