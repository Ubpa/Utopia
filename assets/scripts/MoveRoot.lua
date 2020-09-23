MoveRootSystem = function(schedule)
  local MoveRoot = function(w, singletons, e, idx, cmpts)
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
	translate.value[2] = translate.value[2] - worldtime.deltaTime * math.sin(worldtime.elapsedTime)
  end
  local cLocator = CmptLocator.new(CmptAccessType.new("Ubpa::DustEngine::Translation", AccessMode.WRITE), 1)
  local sLocator = SingletonLocator.new(CmptAccessType.new("Ubpa::DustEngine::WorldTime", AccessMode.LATEST_SINGLETON), 1)
  
  local rootFilter = ArchetypeFilter.new();
  rootFilter.all:add(CmptAccessType.new("Ubpa::DustEngine::MeshFilter", AccessMode.LATEST))
  rootFilter.all:add(CmptAccessType.new("Ubpa::DustEngine::Children", AccessMode.LATEST))
  LuaECSAgency.RegisterEntityJob(
    schedule,
	MoveRoot,
	"MoveRoot",
	rootFilter,
	cLocator,
	sLocator,
	true
  )
end
MoveRootSystemIdx = world.systemMngr:Register("MoveRootSystem", MoveRootSystem)
world.systemMngr:Activate(MoveRootSystemIdx)
