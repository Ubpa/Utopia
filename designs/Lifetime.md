# Lifetime

## DX12APP

### Init

- window
- debug layer
- create DXGI factory
- create `12_0` device
- create fence
- GPURsrcMngrDX12 init
- DescriptorHeapMngr init
- create FrameResourceMngr
  - for each frame resource
    - create allocator
- create command queue
- create main command allocator
- create main graphics command list
- create swap chain
- create swap chain rtv

### Resize

- flush command queue
- swap chain resize buffers
- create rtv for swap chain buffers
- flush command queue

### Destroy

- GPURsrcMngrDX12 clear
- flush command queue
- DescriptorHeapMngr free swap chain rtv
- DescriptorHeapMngr clear

## Editor

### Init

- DX12App init window
- DX12App init Direct3D
- DX12App resize
- ImGUIMngr init
- AssetMngr init
  - set root path `../assets`
  - register asset importer creators
  - import recersively
- InspectorRegistry register `InspectMaterial`
- UDRefl init
- load textures
- load shaders
- create game and scene pipeline
- create game and scene srv and rtv
- create logger
- build worlds
- flush command queue

### OnGameResize

- DX12App resize
- gameRT resize, recreate src, rtv
- game pipeline resize

### OnSceneResize

- DX12App resize
- sceneRT resize, recreate src, rtv
- scene pipeline resize

### Update

- Imgui DX12 new frame
- Imgui Win32 new frame
  - context editor
  - context game
  - context scene
  - shared
- editor
  - imgui new frame
  - dock
  - game window
    - resize
    - image
  - scene window
    - resize
    - image
  - game control (start, stop)
  - editor world update
- game
  - imgui new frame
  - game world update according to the game state
- scene
  - imgui new frame
  - scene world update
- resource management
  - frame resource manager begin frame
  - current frame command allocator reset
  - command list reset with the current frame command allocator
  - update resource in game and scene world
    - for each entity with `MeshFilter` and `MeshRenderer`
      - GPU resource manager register the mesh
      - GPU resource manager register texture2Ds and TextureCubes in the material properties
    - GPU resource manager register the texture2Ds and TextureCubes in the singleton `Skybox`
  - commit update commands to the command queue
  - commit delete function to the current frame resource, and delay unregister it
- rendering prepare for game and scene
  - find camera
  - pipeline begin frame
- render game, scene and editor
  - set imgui context
  - pipeline render
  - imgui render

### Destroy

- DescriptorHeapMngr free srv and rtv

## Pipeline

### BeginFrame

- update render context
  - camera constants
  - collect render objects (entities with `MeshFilter` + `MeshRenderer` + `LocalToWorld`) in all worlds
  - collect lights
  - update skybox
- frameRsrcMngr begin frame
- update shader constant buffers
  - common buffer: camera, lights, objects
  - shader constant buffers

### Render

- reset the current frame command allocator
- frame graph, frame graph resource manager, frame graph executor new frame
- frame graph register
  - resource node
  - move node
  - pass node
- frame graph resource manager register
  - temporal resource
  - resource table
  - imported resource
  - pass resource (state)
- frame graph executor
  - register pass function

### DrawObjects

- draw opaques, transparents
  - if shader changed, set new root signature of shader
  - set root table
  - set vertex buffer, index buffer, primitive topology
  - (optional) set stencil ref
  - set pso
  - draw indexed instance

### EndFrame

- frameRsrcMngr end frame

