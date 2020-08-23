# 三缓冲

这里的缓冲是动态变量缓冲，如 per object constant buffer，per material constant buffer 等

由于 cpu 和 gpu 是异步的，所以至少需要双缓冲才能发挥 cpu 和 gpu 间的并行

> cpu 提交完渲染指令后，无需等待 gpu 完成渲染指令就可以进入下一帧，但当 cpu 需要填充新一帧的数据（各种 constant buffer）到 gpu 时，如果 gpu 还在读取这些缓冲区，则发生了冲突，必须等待 gpu 执行完，然后 gpu 等待 cpu 更新好数据才结束等待。双缓冲就能解决这个问题（这里假设 gpu 是瓶颈或者 cpu 和 gpu 耗时差不多）。

考虑 cpu 和 gpu 每帧耗时差不多的情况，如果只有双缓冲，由于 cpu 提交命令到 gpu 和 gpu 通知 cpu 渲染完成有延时，则可能会造成 cpu 和 gpu 的停顿 [^triple_buffer]，而三缓冲就可以规避这个问题（当 cpu 或 gpu 是明显瓶颈时，则这个延时并无影响）

![image: ../Art/ResourceManagement_TripleBuffering_2x.png](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/Art/ResourceManagement_TripleBuffering_2x.png)

> 图中的 Commit Frame n 和  Frame n callback 就是命令传递造成的延时

总结一下，双缓冲解决了数据冲突，三缓冲解决了命令传递延时

# 参考

[^triple_buffer]: [Metal : Triple Buffer](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/TripleBuffering.html)

