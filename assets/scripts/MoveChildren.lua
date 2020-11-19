MoveChildrenSystem_OnUpdate = function(schedule)
  local MoveChildren = function(w, singletons, e, idx, cmpts)
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
    translate.value[1] = translate.value[1] + worldtime.deltaTime * math.cos(worldtime.elapsedTime)
  end

  local cLocatorTypes = LuaArray_CmptAccessType.new()
  cLocatorTypes:PushBack(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::Translation", Ubpa.UECS.AccessMode.WRITE))
  local cLocator = Ubpa.UECS.CmptLocator.new(cLocatorTypes:ToConstSpan())

  local sLocatorTypes = LuaArray_CmptAccessType.new()
  sLocatorTypes:PushBack(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::WorldTime", Ubpa.UECS.AccessMode.LATEST))
  local sLocator = Ubpa.UECS.SingletonLocator.new(sLocatorTypes:ToConstSpan())

  local childFilter = Ubpa.UECS.ArchetypeFilter.new();
  childFilter.all:add(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::MeshFilter", Ubpa.UECS.AccessMode.LATEST))
  childFilter.all:add(Ubpa.UECS.CmptAccessType.new("Ubpa::Utopia::Parent", Ubpa.UECS.AccessMode.LATEST))
  
  Ubpa.Utopia.LuaECSAgency.RegisterEntityJob(
    schedule,
    MoveChildren,
    "MoveChildren",
    true,
    childFilter,
    cLocator,
    sLocator
  )
end

MoveChildrenSystemID = world.systemMngr.systemTraits:Register("MoveChildrenSystem")
world.systemMngr.systemTraits:RegisterOnUpdate(MoveChildrenSystemID, Ubpa.Utopia.LuaECSAgency.SafeOnUpdate(MoveChildrenSystem_OnUpdate))
world.systemMngr:Activate(MoveChildrenSystemID)
