# 数据驱动渲染

shaderlab 文件包含一个 hlsl

hlsl 内含多个 vs 和 ps

shaderlab 内有多个pass，每个pass 需要指明 vs 和 ps 的名字

注册 shaderlab 时，将各 pass 的 vs 和 ps 编译，所需信息

- hlsl 文件内容
- 宏定义表
- 函数名
- 目标（如 "vs_5_0"）
- include 器

每个函数编译得到一个字节码缓冲区（指针+大小），可使用 `D3DReflect` 得到反射信息 `ID3D12ShaderReflection`。

一个 shaderlab 对应一个根签名，根签名生成需要

- 一组根参数
- 一组静态采样器

