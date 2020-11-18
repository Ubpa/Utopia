# 管理描述符堆

dx12 资源绑定分成两个阶段，中介就是描述符（SRV，UAV，CBV，RTV，DSV，Sampler）

![D3D12ResourceBinding](http://diligentgraphics.com/wp-content/uploads/2016/03/D3D12ResourceBinding-1024x807.png)

描述符堆有四类（DX12）

- CBV / SRV / UAV（`D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV`）
- Sampler（`D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER`）
- RTV（`D3D12_DESCRIPTOR_HEAP_TYPE_RTV`）
- DSV（`D3D12_DESCRIPTOR_HEAP_TYPE_DSV`）

如果想要 GPU 能访问，则堆要标记为 **shader-visible**。只有前两种可被标记为 shader-visible，RTV 和 DSV 只能是 CPU-visible。

shader-visible 描述符堆大小是有限的，因此不能所有描述符都存在 gpu 中，程序需要保证渲染需要的描述符在 gpu 中。

## 概览

描述符堆管理系统主要使用 6 个类

- `VarSizeAllocMngr`: alloc-free 机制的实现类
- `DescriptorHeapAllocation`: 描述符堆的一个分配
- `DescriptorHeapAllocationManager`: 描述符堆分配管理，`VarSizeAllocMngr` 的特化
- `CPUDescriptorHeap`: CPU-only 描述符堆组，内含一堆 `DescriptorHeapAllocationManager` 来实现可扩增的描述符分配
- `DynamicSuballocationsManager`: shader-visible 描述符堆分配管理，`VarSizeAllocMngr` 的特化，与 `DescriptorHeapAllocationManager` 的不同是，`Free` 时不是立马释放，而是在帧结束的时候调用 `ReleaseAllocations` 一起释放
- `GPUDescriptorHeap`：shader-visible 描述符堆，内含 2 个 `DescriptorHeapAllocationManager`，一个用来静态分配，一个用来动态分配。动态分配部分，每个线程自己拿一个 `DynamicSuballocationsManager` 来用，这样就可以做到无锁。注意不可扩增，要及时释放。

## VarSizeAllocMngr

![FreeBlockListManager1](http://diligentgraphics.com/wp-content/uploads/2017/04/FreeBlockListManager1.png)

## DescriptorHeapAllocation

![DescriptorHeapAllocation](http://diligentgraphics.com/wp-content/uploads/2017/04/DescriptorHeapAllocation.png)

## DescriptorHeapAllocationManager

![DescriptorHeapAllocationsManager](http://diligentgraphics.com/wp-content/uploads/2017/04/DescriptorHeapAllocationsManager.png)

## CPUDescriptorHeap

![CPUDescriptorHeap](http://diligentgraphics.com/wp-content/uploads/2017/04/CPUDescriptorHeap.png)

## GPUDescriptorHeap

![GPUDescriptorHeap](http://diligentgraphics.com/wp-content/uploads/2017/04/GPUDescriptorHeap.png)

## 总览

![DescriptorHeaps-BigPicture1](http://diligentgraphics.com/wp-content/uploads/2017/04/DescriptorHeaps-BigPicture1-1024x557.png)

4 个 CPU-only 描述符堆和 2 个 shader-visible 描述符堆

每一个线程会包含一个 `DynamicSuballocationsManager` 

## 用法详解

### 创建 Resource View

创建 texture 的 SRV 示例

1. 从 CPU-only 的描述符堆中分配一个 handle
2. 使用 D3D12 device 来在 handle 上创建 SRV
3. SRV 释放时需要考虑帧同步（比如同时存在三帧，那么就在 3 帧后释放）

### 分配动态描述符

1. 从 `DynamicSuballocationsManager` 中分配 handles 并在其上创建 view
2. 释放时需要考虑帧同步（比如同时存在三帧，那么就在 3 帧后释放）

