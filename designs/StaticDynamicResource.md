# 静态/动态资源

数据划分成两类

- 小型（per shader）：constant buffer
- 大型：mesh，texture

什么是静态和动态呢？

静态是指，这个资源不会变化。而动态则相反，每帧都可能变化

per shader 默认全是动态资源，每帧都变化，这个在 [Shader Constant Buffer](ShaderConstantBuffer.md) 

mesh 和 texture 比较耗，可以更加精细地探究下

mesh 和 texture 类似，就一并说明，不加以区分

用两个 bool 标记资源状态

- dirty：当前帧是否发生了修改
- editable：是否可修改

[D3D12_HEAP_TYPE enumeration](https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type) 提到上传堆和默认堆的用法

静态资源就放在默认堆

对于动态资源，如果只需要读一次，就仅放在上传堆，如果需要读取多次，需要拷贝到默认堆

对于 shader 的 per pass, per material, per object constant buffer等，则只需放在上传堆即可（每一帧都全部重新计算）

而对于 mesh，由于很可能多次使用（阴影 pass + 几何 pass），所以默认会拷贝到上传堆

有下边多种情形

- 未注册 gpu 资源
  - editable：放到默认堆（上传堆直接释放），转为静态
  - non-editable：放到上传堆，并传到默认堆，上传堆资源不释放，转为动态
- 动态
  - non-editable
    - dirty：更新上传堆资源，并传到默认堆，上传堆资源**释放**，转为静态
    - non-dirty：释放上传堆资源，转为静态
  - editable
    - dirty：更新上传堆资源，并传到默认堆，上传堆资源不释放，维持动态
    - non-dirty：维持动态
- 静态：维持静态

![rsrc](assets/StaticDynamicResource/rsrc.png)


