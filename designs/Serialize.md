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

支持容器

- 有序定长：`array`
- 有序变长：`vector`, `list`, `forward_list`, `deque`, `set`, `unordered_set`, `multiset`, `unordered_multiset`
- 元组：`tuple`, `pair`
- 关联：`map`, `unordered_map`, `multimap`, `unordered_multimap`

支持自定义容器（通过特化宏），如

- 有序定长：UGM 的 `vec`, `val` 等

### TODO

- [ ] 对于不支持的类型，用户可自定义函数
- [ ] reserve

## 2. Deserialize

首先遍历一遍，求实体索引映射表（用于实体索引重映射）

按逆过程实现即可

### TODO

- [ ] 用户自行提供 world（注册好 cmpt traits，无 entity）
- [ ] 一次性添加多组件 components
- [ ] reserve
- [ ] 支持 append 模式（索引映射表包含版本）

