MoveChildrenSystem = function(schedule)
  local MoveChildren = function(w, singletons, e, idx, cmpts)
    local type0 = CmptAccessType.new(
	  "Ubpa::Utopia::Translation",
	  AccessMode.WRITE
	)
	
	local type1 = CmptAccessType.new(
	  "Ubpa::Utopia::WorldTime",
	  AccessMode.LATEST_SINGLETON
	)
	
    local cmptPtr      = cmpts:GetCmpt(type0)
    local singletonPtr = singletons:GetSingleton(type1)
	
	local translate = Translation.voidp(cmptPtr:Ptr())
    local worldtime = WorldTime.voidp(singletonPtr:Ptr())
	
	-- 1 x, 2 y, 3 z
	translate.value[1] = translate.value[1] + worldtime.deltaTime * math.cos(worldtime.elapsedTime)
  end
  local cLocator = CmptLocator.new(CmptAccessType.new("Ubpa::Utopia::Translation", AccessMode.WRITE), 1)
  local sLocator = SingletonLocator.new(CmptAccessType.new("Ubpa::Utopia::WorldTime", AccessMode.LATEST_SINGLETON), 1)
  
  local childFilter = ArchetypeFilter.new();
  childFilter.all:add(CmptAccessType.new("Ubpa::Utopia::MeshFilter", AccessMode.LATEST))
  childFilter.all:add(CmptAccessType.new("Ubpa::Utopia::Parent", AccessMode.LATEST))
  
  LuaECSAgency.RegisterEntityJob(
    schedule,
	MoveChildren,
	"MoveChildren",
	childFilter,
	cLocator,
	sLocator,
	true
  )
end
MoveChildrenSystemIdx = world.systemMngr:Register("MoveChildrenSystem", MoveChildrenSystem)
world.systemMngr:Activate(MoveChildrenSystemIdx)
