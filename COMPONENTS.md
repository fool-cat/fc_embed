# FC Embed 组件总览

本文档描述当前仓库的组件与依赖关系。

## 1. 当前组件范围

```text
fc_config.h / core/fc_config_template.h
├── 基础支撑
│   ├── core/fc_compiler.h
│   ├── core/fc_helper.h
│   ├── core/fc_arch.h
│   └── device/fc_type.h
├── core/
│   ├── fc_auto_init
│   ├── fc_fifo
│   ├── fc_pool
│   ├── fc_port
│   ├── fc_stdio
│   ├── fc_log
│   └── fc_trans
└── device/
    ├── fc_sig
    ├── fc_sig_filter
    ├── fc_stp_base
    └── fc_stp_curve
```

## 2. 基础支撑层

| 组件 | 主要文件 | 核心依赖 | 当前实现要点 |
| --- | --- | --- | --- |
| 配置入口 | `core/fc_config_template.h` | 无 | 项目需自备 `fc_config.h`；模板覆盖 `auto_init`、`port`、`log`、`pool`、`trans`、`stdio` 等常用宏。 |
| 编译器兼容与宏工具 | `core/fc_compiler.h` `core/fc_helper.h` | `fc_config.h` | 提供编译器适配、宏拼接、作用域辅助等基础能力。 |
| 架构相关原子区 | `core/fc_arch.h` | `fc_config.h` | 提供 `FC_ATOMIC_ENTER/EXIT/SCOPE` 等原子保护接口。 |
| 设备层公共类型 | `device/fc_type.h` | `fc_config.h` | 统一 `fc_time_t`、时间函数和断言宏入口。 |

## 3. `core/` 组件

| 组件 | 主要文件 | 核心依赖 | 当前实现要点 |
| --- | --- | --- | --- |
| `fc_auto_init` | `core/fc_auto_init.c/.h` | `fc_compiler` `fc_helper` `fc_config` | 4 段自动初始化框架：`ENV / CLOCK / DEVICE / APP`，支持按优先级注册。 |
| `fc_fifo` | `core/fc_fifo.h` | `fc_compiler` `fc_config` | 2 的幂大小字节流环形缓冲区；支持普通读写、覆盖写与线性零拷贝窗口；C++ 模板封装已合并至同一头文件。 |
| `fc_pool` | `core/fc_pool.c/.h` | `fc_compiler` `fc_arch` `fc_config` | 固定块内存池；支持单块分配、连续块、链式块与 FIFO 化已用块。 |
| `fc_port` | `core/fc_port.c/.h` | `fc_fifo` `fc_config` | 多 ring buffer 端口抽象；提供 `stdin/stdout` 默认对象与 `phy` 回调。 |
| `fc_stdio` | `core/fc_stdio.c/.h` `core/utils/*.c` | 无强制运行时依赖 | 轻量 `printf` 核心；`fc_stdio.c` 直接聚合 `core/utils/` 下的实现文件。 |
| `fc_log` | `core/fc_log.c/.h` | `fc_pool` `fc_stdio` `fc_auto_init` | 日志框架，支持等级控制、ANSI 颜色、池化缓冲与默认延迟输出后端。 |
| `fc_trans` | `core/fc_trans.c/.h` | `fc_fifo` `fc_port` `fc_helper` | 基于分页标记 `\033[?;<num>m` 的单通道聚合传输；头文件已注明“暂时建议不使用”。 |

## 4. `device/` 组件

| 组件 | 主要文件 | 核心依赖 | 当前实现要点 |
| --- | --- | --- | --- |
| `fc_type` | `device/fc_type.h` | `fc_config.h` | 设备层公共时间与断言约定。 |
| `fc_sig` | `device/fc_sig.c/.h` | `fc_type.h` | 二值信号抽象层，负责采样、稳态维护与触发回调。 |
| `fc_sig_filter` | `device/fc_sig_filter.c/.h` | `fc_sig` | 信号滤波策略集合，当前实现包含比例、重复等滤波器。 |
| `fc_stp_base` | `device/fc_stp_base.c/.h` | `core/fc_arch.h` `fc_config.h` | 步进电机底层执行骨架，负责 step/dir/ioctl、速度心跳和位置推进。 |
| `fc_stp_curve` | `device/fc_stp_curve.c/.h` | `fc_stp_base` | 步进曲线模型集合，当前实现包含 `ladder`、`trapezoid`、`s_curve`、`s_curve_tri_acc`。 |

## 5. 依赖关系图

```text
fc_compiler / fc_helper / fc_arch / fc_config
├── fc_auto_init
├── fc_fifo
│   ├── fc_port
│   │   └── fc_trans
├── fc_pool
│   └── fc_log
├── fc_stdio
│   └── fc_log
└── device/fc_type
    ├── fc_sig
    │   └── fc_sig_filter
    └── fc_stp_base
        └── fc_stp_curve
```

## 6. 常见组合

- 最小基础能力:
  `fc_compiler + fc_helper + fc_arch + fc_auto_init`
- 基础收发:
  `fc_fifo + fc_port`
- 格式化输出:
  `fc_stdio`
- 完整日志链:
  `fc_stdio + fc_pool + fc_log`
- 信号采样与去抖:
  `fc_type + fc_sig + fc_sig_filter`
- 步进底层执行:
  `fc_stp_base + fc_stp_curve`

## 7. 配置要点

项目需要提供 `fc_config.h`。按当前组件，常见配置项包括:

```c
#ifndef _FC_CONFIG_H_
#define _FC_CONFIG_H_

#define USE_FC_AUTO_INIT 1

#define FC_USE_STD_MEMCPY 1

#define PORT_RB_NUM 8
#define STDOUT_RB0_LOG2_SIZE 12
#define STDIN_RB0_LOG2_SIZE 8

#define FC_LOG_ENABLE 1
#define FC_LOG_LINE_SIZE 128
#define FC_LOG_POOL_TOTAL_SIZE (4 * 1024)

#define FC_FOOL_ENABLE_DYNAMIC_POOL_ALLOC 0

#define FC_DIVISION_NUM_MAX_LEN 4
#define FC_DIVISION_NUM_MAX 9999

#define XF_USE_LLI 1
#define XF_USE_FP 1

#endif
```

## 8. 构建说明

- `fc_fifo` 为头文件实现，集成时通常只需包含头文件。
- `fc_stdio.c` 会直接 `#include` `core/utils/` 下的实现文件，不需要把这些小 `.c` 单独逐个加入工程。
- 自动初始化依赖编译器段机制；不同工具链的链接脚本或构建选项需要配套支持。

## 9. 一句话总结

当前 `fc_embed` 由两部分构成:

- `core/` 负责基础设施、缓冲、日志与传输
- `device/` 负责设备公共类型、信号抽象与步进底层执行
