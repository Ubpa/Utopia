# 序列化

引擎核心使用 [USRefl](https://github.com/Ubpa/USRefl) 来进行序列化，用户亦可自定义特定组件的序列化方法

## 1. Serialize

首先支持基本类型

- `float`, `double` 
- `bool` 
- `uint{8|16|32|64}_t`, `int{8|16|32|64}_t` 
- `nullptr` 

特殊的简单类型还有

- Entity：只记录 index（非稳定），只用于反序列化
- asset：非 `nullptr` 指针就查查是否是 asset，如果是就记录其 GUID

将支持容器

- vector, array
- set, unordered_set
- tuple, pair
- map, unordered_map

将支持自定义容器（通过特化宏）

对于不支持的类型，用户可自定义函数

## 2. Deserialize

world 需要注册 traits

其余按逆过程实现即可

