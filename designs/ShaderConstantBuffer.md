# Shader Constant Buffer

每个 shader 需要一些 constant buffer

假设每帧都会变化

引擎会给每个 shader 提供任意个可手动调整大小的 constant buffer

暂定的使用策略如下

- 帧资源对每个 shader 有专属的一个 constant buffer
- constant buffer 里会填充 per camera，per material，per object 的数据
- 每帧都首先统计各个 shader 涉及的 material 个数，以及各 material 的 object 个数，从而计算出 constant buffer 的大小，然后填充数据（每帧都全部重新计算）

> **示例** 
>
> 假设
>
> - 2 个 shader
> - 每个 shader 各有 2 个 material
> - 每个 material 各有 2 个 object
> - 2 个相机
>
> > 命名和大小
> >
> > - shader0
> >   - material00 (size_0_m)
> >     - object000 (size_0_o)
> >     - object001 (size_0_o)
> >   - material01 (size_0_m)
> >     - object010 (size_0_o)
> >     - object011 (size_0_o)
> > - shader1
> >   - material10 (size_1_m)
> >     - object100 (size_1_o)
> >     - object101 (size_1_o)
> >   - material11 (size_1_m)
> >     - object110 (size_1_o)
> >     - object111 (size_1_o)
> > - camera0, cmaera1
>
> 那么 constant buffer 的布局为
>
> ```
> [shader0 constant buffer]
> camera0
> camera1
> material00
> material01
> object000
> object001
> object010
> object011
> 
> [shader1 constant buffer]
> camera0
> camera1
> material10
> material11
> object100
> object101
> object110
> object111
> ```

shader 用的 constant buffer 是 [UDX12::DynamicConstantBuffer](https://github.com/Ubpa/UDX12/tree/master/include/UDX12) 

帧资源里塞 [ShaderCBMngrDX12](../include/DustEngine/Render/DX12/ShaderCBMngrDX12.h) 

未来可能考虑多个shader间搞一个共享 buffer

