# 多线程

cpu 的工作如下

1. world update (**parallel**)
2. wait and world -> GPU (flush)
3. frame graph
   1. register resource nodes
   2. register pass nodes
   3. compile frame graph
   4. **parallelly** execute frame graph (avoid conflict for some operations)
   5. sequentially commit comand lists to the command queue

步骤 1 和 3 内部是并行的，步骤 2 过程中 world 不能修改

- level 1：直接串行执行各步骤，且 3.4 串行执行

- level 2：3.4 并行
- level 3：1-2 和 3 进行流水线并行模式

- level 4：为了 1-2 也能执行流水线并行模式，分3级上传数据
  - 1级：上传 dynamic 大数据（网格，贴图等），拷贝小数据（pass，material，object constant）
  - 2级：将需要上传的 static 数据标记为锁定状态（不允许删除），此时可让步骤 1 运行，然后上传这些数据，并逐步解锁
  - 3级：上传小数据

level 2 肯定要实现

level 3 在之后可以实现

level 4 会对整个框架有较大影响

