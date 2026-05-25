# fc_embed

`fc_embed` 是一个面向嵌入式场景的轻量基础组件库。本文档只做仓库入口导航。

## 快速入口

- 组件依赖总览: [COMPONENTS.md](COMPONENTS.md)
- 版本定义: [core/fc_version.h](core/fc_version.h)
- 配置模板: [core/fc_config_template.h](core/fc_config_template.h)
- `stdio` 辅助源码编译说明: [core/utils/readme.md](core/utils/readme.md)
- 文档修订记录: [docs/records/2026-04-14-doc-scope.md](docs/records/2026-04-14-doc-scope.md)

## `core/` 模块导航

`core/` 提供通用基础能力，当前内容主要包含编译器适配、初始化、缓冲区、内存池、IO、日志与分页传输组件。

| 模块 | 作用 | 导航 |
| --- | --- | --- |
| `fc_version` | 包级与组件级版本号定义 | [fc_version.h](core/fc_version.h) |
| `fc_compiler` / `fc_helper` / `fc_arch` | 编译器兼容、宏工具、原子区与底层辅助能力 | [fc_compiler.h](core/fc_compiler.h) · [fc_helper.h](core/fc_helper.h) · [fc_arch.h](core/fc_arch.h) |
| `fc_auto_init` | 分阶段、按优先级自动初始化框架 | [说明文档](core/fc_auto_init.md) · [头文件](core/fc_auto_init.h) |
| `fc_fifo` | 字节流环形缓冲区，支持零拷贝窗口与 C++ 定长封装 | [说明文档](core/fc_fifo.md) · [头文件](core/fc_fifo.h) |
| `fc_pool` | 固定块/链式块内存池，支持 FIFO 化已用块管理 | [说明文档](core/fc_pool.md) · [头文件](core/fc_pool.h) |
| `fc_port` | 基于 FIFO 的多缓冲端口抽象，可接标准输入输出 | [说明文档](core/fc_port.md) · [头文件](core/fc_port.h) |
| `fc_stdio` | 轻量 `printf`/`fprintf`/`snprintf` 风格输出层 | [说明文档](core/fc_stdio.md) · [头文件](core/fc_stdio.h) · [utils 目录](core/utils/readme.md) |
| `fc_log` | 日志框架，支持等级控制、池化缓冲与默认后端 | [说明文档](core/fc_log.md) · [头文件](core/fc_log.h) |
| `fc_trans` | 单通道分页聚合传输接口 | [头文件](core/fc_trans.h) · [实现](core/fc_trans.c) |

## `device/` 模块导航

当前 `device/` 内容主要集中在设备公共类型、信号抽象与步进底层执行。

| 模块 | 作用 | 导航 |
| --- | --- | --- |
| `fc_type` | 设备层公共类型、时间函数与断言宏入口 | [fc_type.h](device/fc_type.h) |
| `fc_sig` | 二值信号抽象层，负责稳态采样、触发和回调调度 | [说明文档](device/fc_sig.md) · [头文件](device/fc_sig.h) |
| `fc_sig_filter` | 信号滤波策略集合与选型说明 | [说明文档](device/fc_sig_filter.md) · [头文件](device/fc_sig_filter.h) |
| `fc_stp_base` | 步进电机底层运动骨架，负责心跳、位移与速度执行 | [说明文档](device/fc_stp_base.md) · [头文件](device/fc_stp_base.h) |
| `fc_stp_curve` | 步进曲线模型与参数说明 | [说明文档](device/fc_stp_curve.md) · [头文件](device/fc_stp_curve.h) |

## 推荐阅读顺序

1. 先看 [COMPONENTS.md](COMPONENTS.md) 了解整体依赖关系。
2. 想看基础能力时，优先从 [fc_auto_init](core/fc_auto_init.md)、[fc_fifo](core/fc_fifo.md)、[fc_pool](core/fc_pool.md)、[fc_port](core/fc_port.md)、[fc_stdio](core/fc_stdio.md)、[fc_log](core/fc_log.md) 开始。
3. 想看设备信号链路时，先读 [fc_sig](device/fc_sig.md) 与 [fc_sig_filter](device/fc_sig_filter.md)。
4. 想看步进底层链路时，建议按 [fc_stp_base](device/fc_stp_base.md) -> [fc_stp_curve](device/fc_stp_curve.md) 的顺序阅读。
