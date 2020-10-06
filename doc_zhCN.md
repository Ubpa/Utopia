# 文档

Utopia 在设计上大体参考了 Unity，熟悉 Untiy 的朋友应该能很快上手。

## 使用

运行 Utopia_test_App_Editor 即可启动 Editor

你可以在该项目中添加组件和系统来扩展功能

## 问题

Utopia 还处于快速开发中，因此其功能在易用性上还是有一定欠缺的。

目前的主要问题有

- 资源管理方案（资产（创建/删除/编辑/刷新）、临时、动态、更新/删除）
- 脚本

解决的办法

- 资产：通过重新启动软件，实现对资产的刷新
- 脚本：暂时不能使用，通过写 C++ 完成需求

## ECS

底层框架是用 ECS 构成的，在 Unity 中通过编写组件实现功能的扩展，这里也类似，通过编写组件和系统实现对引擎的功能扩展。

编辑器由 3 个 World 构成（Game，Scene，Editor），所有的引擎功能皆由组件，系统和一些辅助类实现。`Editor` 类将三个 world 都暴露出来了，用户可以通过修改 world 内的 E、C、S 来定制自己的功能。

当然，还可以通过脚本的形式去扩展功能，但目前脚本方案还不够完善，暂不推荐。

ECS 的编写方式，参考 [UECS](https://github.com/Ubpa/UECS) 的使用示例和源码注释，也可以看看 Unity 的 [ECS 文档](https://docs.unity3d.com/Packages/com.unity.entities@0.14/manual/index.html)了解大致思路

目前的 ECS 图如下

![Alt text](https://g.gravizo.com/source/gravizo_mask_ecs?https%3A%2F%2Fraw.githubusercontent.com%2FUbpa%2FUtopia%2Fmaster%2Fdoc_zhCN.md)

<details>  
<summary>ECS graphviz source code</summary>
gravizo_mask_ecs
digraph G {
  node [
    fontcolor = "white"
    fontname = "consolas"
    style = "filled"
  ]
  subgraph "Component Nodes" {
    node [
      color = "#6597AD"
      shape = "ellipse"
    ]
    "struct Ubpa::Utopia::LocalToParent"
    "struct Ubpa::Utopia::Children"
    "struct Ubpa::Utopia::Rotation"
    "struct Ubpa::Utopia::Roamer"
    "struct Ubpa::Utopia::LocalToWorld"
    "struct Ubpa::Utopia::WorldToLocal"
    "struct Ubpa::Utopia::Parent"
    "struct Ubpa::Utopia::RotationEuler"
    "struct Ubpa::Utopia::Scale"
    "struct Ubpa::Utopia::Translation"
    "struct Ubpa::Utopia::Camera"
  }
  subgraph "Singleton Nodes" {
    node [
      color = "#BFB500"
      shape = "ellipse"
    ]
    "struct Ubpa::Utopia::WorldTime"
    "struct Ubpa::Utopia::Input"
  }
  subgraph "System Function Nodes" {
    node [
      color = "#F79646"
      shape = "box"
    ]
    "WorldTimeSystem"
    "LocalToParentSystem"
    "WorldToLocalSystem"
    "RotationEulerSystem"
    "TRSToLocalToParentSystem"
    "TRSToWorldToLocalSystem"
    "CameraSystem"
    "InputSystem"
    "RoamerSystem"
  }
  subgraph "LastFrame Edges" {
    edge [ color = "#60C5F1" ]
  }
  subgraph "Write Edges" {
    edge [ color = "#F47378" ]
    "TRSToLocalToParentSystem" -> "struct Ubpa::Utopia::LocalToParent"
    "WorldTimeSystem" -> "struct Ubpa::Utopia::WorldTime"
    "CameraSystem" -> "struct Ubpa::Utopia::Camera"
    "LocalToParentSystem" -> "struct Ubpa::Utopia::LocalToWorld"
    "WorldToLocalSystem" -> "struct Ubpa::Utopia::WorldToLocal"
    "RotationEulerSystem" -> "struct Ubpa::Utopia::Rotation"
    "TRSToWorldToLocalSystem" -> "struct Ubpa::Utopia::LocalToWorld"
    "InputSystem" -> "struct Ubpa::Utopia::Input"
    "RoamerSystem" -> "struct Ubpa::Utopia::Translation"
    "RoamerSystem" -> "struct Ubpa::Utopia::Rotation"
  }
  subgraph "Latest Edges" {
    edge [ color = "#6BD089" ]
    "struct Ubpa::Utopia::Translation" -> "TRSToLocalToParentSystem"
    "struct Ubpa::Utopia::Children" -> "LocalToParentSystem"
    "struct Ubpa::Utopia::LocalToWorld" -> "WorldToLocalSystem"
    "struct Ubpa::Utopia::RotationEuler" -> "RotationEulerSystem"
    "struct Ubpa::Utopia::Scale" -> "TRSToLocalToParentSystem"
    "struct Ubpa::Utopia::Rotation" -> "TRSToLocalToParentSystem"
    "struct Ubpa::Utopia::Scale" -> "TRSToWorldToLocalSystem"
    "struct Ubpa::Utopia::Translation" -> "TRSToWorldToLocalSystem"
    "struct Ubpa::Utopia::Rotation" -> "TRSToWorldToLocalSystem"
    "struct Ubpa::Utopia::Roamer" -> "RoamerSystem"
    "struct Ubpa::Utopia::WorldTime" -> "RoamerSystem"
    "struct Ubpa::Utopia::Input" -> "RoamerSystem"
  }
  subgraph "Order Edges" {
    edge [ color = "#00A2E8" ]
    "TRSToLocalToParentSystem" -> "LocalToParentSystem"
    "TRSToWorldToLocalSystem" -> "LocalToParentSystem"
  }
  subgraph "All Edges" {
    edge [
      arrowhead = "crow"
      color = "#C785C8"
      style = "dashed"
    ]
  }
  subgraph "Any Edges" {
    edge [
      arrowhead = "diamond"
      color = "#C785C8"
      style = "dashed"
    ]
  }
  subgraph "None Edges" {
    edge [
      arrowhead = "odot"
      color = "#C785C8"
      style = "dashed"
    ]
    "LocalToParentSystem" -> "struct Ubpa::Utopia::Parent"
    "TRSToWorldToLocalSystem" -> "struct Ubpa::Utopia::LocalToParent"
  }
  node [ color=".0 .0 .0" ];
}
gravizo_mask_ecs
</details>

## ImGui

Utopia 使用了 ImGui 作为 UI 框架，并且为每一个 world 都实现了一套 imgui 的 context，用户可以在 ECS 的同步点（`World::AddCommand()`）处实现对 UI 的处理（未来可能将 UI 逻辑放到 Job System 中执行，通过一个ECS 的“写组件”机制实现，一方面避免了写冲突，另一方面可以跟其他 job 并行）

## 资产

Utopia 采用了类似 Unity 的资产管理方案，目前通过 GUID （在同名的 meta 文件内）来实现对资产的引用，这样用户可以随意修改文件名，挪到文件位置，而不会造成引用失效（通过路径方式指定就会依赖于文件名）。

Utopia 目前支持的资产分以下几种

- 模型：目前支持 *.obj, *.ply，仅用于提供网格，目前只支持三角网格，如果模型不自带纹理坐标，法向，切向量这些，引擎会自动生成，并且不可修改。支持在系统中生成动态网格，但相应的 GPU 资源的释放目前还没做，用户也需要手动释放这个动态网格。
- HLSL：内含 shader 代码
- shader：类 ShaderLab，新增了对根参数的描述，要求将函数实现写在一个 HLSL 内，目前不支持跨平台跨特性
- material：材质，用于对 shader 的各 property 进行赋值
- image：支持 *.jpg, *.png, *.bmp, *.hdr，目前引擎内部直接把所有图形都转成 float 的形式，没有压缩，所以内存显存占用比较大，后续再做好图像压缩问题
- texture 2D：对 image 的引用，用于给 material 内的 property（2D）赋值
- texture cube：对 6 个 image / 1 个 image 的引用，用于给 material 内的 property（Cube）赋值
- script：支持 lua，但目前整个脚本方案还不够完善，不使用

资产管理目前支持的功能仅包含

- 启动时导入在 assets 文件夹内的所有资产
- 在编辑器内可以浏览资产，并可以拖动资产以在 inspector 内赋值

如果需要新增一些资产，需要实现以下步骤（未来为简化此部分的操作）

- 将资产放到文件夹内
- 如果是贴图或HLSL，还需要编写相应的 texture 文件或 shader 文件（由于这些文件需要引用其他文件，所以还需要手动为需要引用的文件编写相应的 meta 文件）
- 启动编辑器，完成新资产导入

## 渲染

目前，仅支持 DX12，也就意味着目前只支持 Windows

![Alt text](https://g.gravizo.com/source/gravizo_mask_fg?https%3A%2F%2Fraw.githubusercontent.com%2FUbpa%2FUtopia%2Fmaster%2Fdoc_zhCN.md)

<details>  
<summary>Frame Graph graphviz source code</summary>
gravizo_mask_fg
digraph G {
  node [
    fontcolor = "white"
    fontname = "consolas"
    style = "filled"
  ];
  subgraph "Resource Nodes" {
    node [
      color = "#F79646"
      shape = "box"
    ];
    "GBuffer0"
    "GBuffer1"
    "GBuffer2"
    "Defer Lighted"
    "Defer Lighted with Sky"
    "Scene"
    "Present"
    "Defer Depth Stencil"
    "Forward Depth Stencil"
    "Irradiance Map"
    "PreFilter Map"
  }
  subgraph "Pass Nodes" {
    node [
      color = "#6597AD"
      shape = "ellipse"
    ]
    "GBuffer Pass"
    "IBL"
    "Defer Lighting"
    "Skybox"
    "Forward"
    "Post Process"
  }
  subgraph "Read Edges" {
    edge [ color = "#9BBB59" ]
    "GBuffer0" -> "Defer Lighting"
    "Irradiance Map" -> "Forward"
    "GBuffer1" -> "Defer Lighting"
    "GBuffer2" -> "Defer Lighting"
    "Defer Depth Stencil" -> "Defer Lighting"
    "Irradiance Map" -> "Defer Lighting"
    "PreFilter Map" -> "Defer Lighting"
    "Defer Depth Stencil" -> "Skybox"
    "PreFilter Map" -> "Forward"
    "Scene" -> "Post Process"
  }
  subgraph "Write Edges" {
    edge [ color = "#ED1C24" ]
    "GBuffer Pass" -> "GBuffer0"
    "GBuffer Pass" -> "GBuffer1"
    "GBuffer Pass" -> "GBuffer2"
    "GBuffer Pass" -> "Defer Depth Stencil"
    "Defer Lighting" -> "Defer Lighted"
    "IBL" -> "Irradiance Map"
    "IBL" -> "PreFilter Map"
    "Skybox" -> "Defer Lighted with Sky"
    "Forward" -> "Forward Depth Stencil"
    "Forward" -> "Scene"
    "Post Process" -> "Present"
  }
  subgraph "Move Edges" {
    edge [ color = "#F79646" ]
    "Defer Lighted" -> "Defer Lighted with Sky"
    "Defer Lighted with Sky" -> "Scene"
    "Defer Depth Stencil" -> "Forward Depth Stencil"
  }
  node [ color=".0 .0 .0" ];
}
gravizo_mask_fg
</details>

> 椭圆的是 pass，矩形的是 resource，绿边是读，红边是写，橙色边代表 move（仅起到换名作用）
>
> 一个资源，生命期为 move in -> write -> multi-read -> move out
>
> 由此，pass 的执行顺序可由 resource 的生命期而确定

Utopia 提供了类 ShaderLab 的声明语言（文件后缀为 shader），你可以在里边提供多个 pass，每个 pass 通过 LightMode 来决定其在 pipeline 的位置（Deferred 表示处于 Defer Lighting，Froward 表示处于 Forward），然后可以用 Queue 指定其在 pipeline pass 内的渲染顺序（0-2499 为非透明物体，2500以上为透明物体，同 queue 则从后往前绘制）

