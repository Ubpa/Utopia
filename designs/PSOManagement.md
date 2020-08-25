# PSO 管理

首先说明下本引擎中 hlsl file, shader, material, pass, root signature, pso 的概念与联系

- hlsl file：需要包含 vs 和 ps
- shader：hlsl file + vs & ps entry + target + name
- material：shader + 性质（特定的贴图，特定的 float4 等）
- pass：一系列渲染操作（相同 Render Target），如 geometry pass，内部支持多个 shader，每个 shader 多个  material，每个 material 多个 object（每个 object 是一个 submesh）
- root signature：根参数（shader 的输入格式），基本上是 1 对 1 的关系，也可以是多 shader 共用 1 个 root signature
- pso：渲染配置，在一个 pass 里，由 object 而定（不同的 mesh layout 可对应不同的 pso）

首先生成引擎支持的所有 mesh layout（大概就 100 个以内，目前是 16 个）

然后根据需要，动态/提前生成对应不同 mesh layout 的 pso

