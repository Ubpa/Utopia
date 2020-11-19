MoveRootSystem_OnUpdate = function(schedule)
  local MoveRoot = function(w, singletons, e, idx, cmpts)
    local type0 = Ubpa.UECS.CmptAccessType.new(
      "Ubpa::Utopia::Translation",
      Ubpa.UECS.AccessMode.WRITE
    )
  
    local type1 = Ubpa.UECS.CmptAccessType.new(
      "Ubpa::Utopia::WorldTime",
      Ubpa.UECS.AccessMode.LATEST
    )
    
    local cmptPtr      = cmpts:GetCmpt(type0)
    local singletonPtr = singletons:GetSingleton(type1)
    
    local translate = Ubpa.Utopia.Translation.voidp(cmptPtr:Ptr())
    local worldtime = Ubpa.Utopia.WorldTime.voidp(singletonPtr:Ptr())
  
    -- 1 x, 2 y, 3 z
    translate.value[2] = translate.value[2] - worldtime.deltaTime * math.sin(worldtime.elapsedTime)
  end
  
  local cLocatorTypes = LuaArray_CmptAccessType.new()
  cLocatorTypes:PushBack(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::Translation", Ubpa.UECS.AccessMode.WRITE))
  local cLocator = Ubpa.UECS.CmptLocator.new(cLocatorTypes:ToConstSpan())
  
  local sLocatorTypes = LuaArray_CmptAccessType.new()
  sLocatorTypes:PushBack(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::WorldTime", Ubpa.UECS.AccessMode.LATEST))
  local sLocator = Ubpa.UECS.SingletonLocator.new(sLocatorTypes:ToConstSpan())
  
  local rootFilter = Ubpa.UECS.ArchetypeFilter.new();
  rootFilter.all:add(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::MeshFilter", Ubpa.UECS.AccessMode.LATEST))
  rootFilter.all:add(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::Children", Ubpa.UECS.AccessMode.LATEST))
  
  Ubpa.Utopia.LuaECSAgency.RegisterEntityJob(
    schedule,
    MoveRoot,
    "MoveRoot",
    true,
    rootFilter,
    cLocator,
    sLocator
  )
end

MoveRootSystemID = world.systemMngr.systemTraits:Register("MoveRootSystem")
world.systemMngr.systemTraits:RegisterOnUpdate(MoveRootSystemID, Ubpa.Utopia.LuaECSAgency.SafeOnUpdate(MoveRootSystem_OnUpdate))
world.systemMngr:Activate(MoveRootSystemID)
