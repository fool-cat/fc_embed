# fc_embed

> 个人嵌入式开发库 - 提供轻量级、高性能的嵌入式系统基础组件

![License](https://img.shields.io/github/license/fool-cat/fc_embed)
![Language](https://img.shields.io/badge/language-C-blue.svg)
![Platform](https://img.shields.io/badge/platform-Cortex--M-orange.svg)

## 📖 简介

`fc_embed` 是一个面向 ARM Cortex-M 系列微控制器的嵌入式基础库，提供了一系列高性能、可配置的核心组件，帮助开发者快速构建嵌入式应用。

### 核心特性

- 🚀 **高性能**：关键组件如 FIFO 采用无锁设计，内存池提供 O(1) 分配
- 🔧 **模块化**：组件独立可选，按需集成
- 💾 **内存高效**：精细的内存管理，适合资源受限的嵌入式环境
- 🎯 **易于移植**：支持 Keil、IAR、GCC 等主流编译器
- 📦 **CMSIS-Pack 支持**：支持通过 Keil 包管理器安装

## 🧩 组件架构

```
fc_embed 核心组件
├── Common          编译器抽象、辅助宏、配置管理
├── Auto_Init       可选的自动初始化框架（4级优先级）
├── FIFO            无锁环形缓冲区（单生产者/单消费者）
├── Memory_Pool     O(1)固定大小内存池管理器
├── Port            多缓冲区 I/O 抽象层（类 SEGGER RTT）
├── Transport       多路复用传输层（ANSI 转义序列）
├── StdIO           简化的标准 I/O（printf 家族）
└── Logger          多级日志系统（带颜色、RTT 支持）
```

### 依赖关系

```
Common (基础)
  ├─> FIFO (头文件)
  ├─> Memory_Pool
  └─> StdIO

FIFO → Port (多缓冲区 I/O)

Port + FIFO → Transport (多路复用)

Memory_Pool + StdIO → Logger (日志系统)

Auto_Init (可选，提供自动初始化)
```

## 📦 安装方式

### 方式 1: CMSIS-Pack（推荐）

1. 从 [Releases](https://github.com/fool-cat/fc_embed/releases) 下载最新的 `.pack` 文件
2. 在 Keil MDK 中导入：`Pack Installer` → `Import` → 选择 `.pack` 文件
3. 在项目中选择需要的组件：`Manage Run-Time Environment` → `fc_embed`

### 方式 2: 源码集成

```bash
git clone https://github.com/fool-cat/fc_embed.git
cd fc_embed
git checkout develop  # 或 CMSIS-Pack 分支
```

将 `core/` 目录下的文件添加到你的项目中。

## 🔧 快速开始

### 1. 基础配置

复制配置模板并根据需求修改：

```c
// 基于 core/fc_config_template.h 创建 fc_config.h
#define FC_LOG_ENABLE           1
#define FC_LOG_LEVEL           FC_LOG_ALL
#define USE_FC_AUTO_INIT       1  // 启用自动初始化
```

### 2. FIFO 环形缓冲区

```c
#include "fc_fifo.h"

// 定义 FIFO
fc_fifo_t fifo;
uint8_t buffer[256];

// 初始化
fc_fifo_init(&fifo, buffer, sizeof(buffer));

// 写入数据
uint8_t data[] = "Hello";
fc_fifo_write(&fifo, data, sizeof(data));

// 读取数据
uint8_t recv[10];
size_t len = fc_fifo_read(&fifo, recv, sizeof(recv));
```

**特性：**
- 无锁设计（单生产者/单消费者）
- 基于 Linux kfifo 实现
- 支持批量读写、零拷贝操作

### 3. 内存池管理

```c
#include "fc_pool.h"

// 定义内存池
fc_pool_t pool;
uint8_t pool_mem[1024];

// 初始化（每块 64 字节）
fc_pool_init(&pool, pool_mem, sizeof(pool_mem), 64);

// 分配内存
void *ptr = fc_pool_alloc(&pool);

// 释放内存
fc_pool_free(&pool, ptr);
```

**特性：**
- O(1) 时间复杂度
- 固定大小块分配
- 支持内存链式扩展

### 4. 日志系统

```c
#include "fc_log.h"

// 初始化日志（使用自动初始化时无需手动调用）
fc_log_pool_init();

// 使用日志
FC_LOG_I("系统启动");
FC_LOG_W("警告：温度过高 %d°C", temp);
FC_LOG_E("错误：传感器无响应");
```

**特性：**
- 多级日志（ERROR/WARN/INFO/DEBUG/VERBOSE）
- 支持颜色输出
- 集成 SEGGER RTT 支持

### 5. Port I/O 抽象

```c
#include "fc_port.h"

// 默认端口已自动初始化（使用自动初始化时）
// 或手动初始化
fc_default_port_init();

// 重定向 printf 到端口
printf("输出到调试端口\n");

// 或直接使用端口 API
fc_port_printf(&fc_stdout, "格式化输出: %d\n", value);
```

### 6. 自动初始化框架

```c
#include "fc_auto_init.h"

// 定义初始化函数
void my_device_init(void) {
    // 设备初始化代码
}

// 注册到自动初始化（DEVICE 阶段，优先级 100）
INIT_EXPORT_DEVICE(my_device_init, 100);

// 在 main 中调用初始化
int main(void) {
    fc_section_init_env();      // 环境初始化（main 之前）
    fc_section_init_clock();    // 时钟初始化
    fc_section_init_device();   // 设备初始化
    fc_section_init_app();      // 应用初始化
    
    while(1) {
        // 主循环
    }
}
```

## 📚 组件详解

### FIFO 环形缓冲区

基于 Linux kfifo 的无锁环形队列实现，适用于单生产者/单消费者场景。

**主要 API：**
- `fc_fifo_init()` - 初始化
- `fc_fifo_write()` / `fc_fifo_read()` - 批量读写
- `fc_fifo_write_byte()` / `fc_fifo_read_byte()` - 单字节读写
- `fc_fifo_peek()` - 查看但不取出
- `fc_fifo_get_write_region()` - 零拷贝写入

### Memory Pool 内存池

O(1) 复杂度的固定大小内存池，支持链式内存块扩展。

**主要 API：**
- `fc_pool_init()` - 初始化
- `fc_pool_alloc()` - 分配内存块
- `fc_pool_free()` - 释放内存块
- `fc_pool_chain()` - 链接多个内存池

### Port I/O 层

类似 SEGGER RTT 的多缓冲区 I/O 抽象，支持标准 I/O 重定向。

**主要 API：**
- `fc_port_printf()` - 格式化输出
- `fc_port_write()` / `fc_port_read()` - 原始读写
- 标准 I/O 重定向支持（printf/scanf）

### Logger 日志系统

基于内存池的轻量级日志系统，支持多级过滤和颜色输出。

**日志级别：**
```c
FC_LOG_E()  // ERROR   - 错误
FC_LOG_W()  // WARN    - 警告
FC_LOG_I()  // INFO    - 信息
FC_LOG_D()  // DEBUG   - 调试
FC_LOG_V()  // VERBOSE - 详细
```

### Transport 传输层

使用 ANSI 转义序列实现的多路复用传输层，可在单个物理通道上传输多个逻辑通道。

### Auto Init 自动初始化

4 级优先级的自动初始化框架：
- **ENV** - 环境层（main 之前/构造函数）
- **CLOCK** - 时钟配置
- **DEVICE** - 设备初始化
- **APP** - 应用初始化

## 🛠️ 编译器支持

- ✅ ARM Compiler 5/6 (Keil MDK)
- ✅ IAR EWARM
- ✅ GCC (ARM Embedded)
- ✅ MSVC (Windows 测试)

## 📝 配置说明

所有配置项在 `fc_config.h` 中定义（基于 `fc_config_template.h` 创建），主要配置包括：

```c
// 自动初始化
#define USE_FC_AUTO_INIT        1

// 日志配置
#define FC_LOG_ENABLE           1
#define FC_LOG_LEVEL           FC_LOG_ALL
#define FC_LOG_COLOR_ENABLE    1

// 内存池配置
#define FC_LOG_POOL_TOTAL_SIZE  (4 * 1024)
#define FC_LOG_ALLOC_BLOCK_SIZE 128

// Port 配置
#define FC_PORT_NUM             8
#define FC_PORT0_SIZE           1024
```

## 🧪 测试示例

项目提供了多个测试示例，位于 `test/` 目录：

- `test_ring_fifo.cpp` - FIFO 环形队列测试
- `test_stdio.cpp` - StdIO 测试
- `test_transport.cpp` - Transport 传输层测试
- `test_auto_init.cpp` - 自动初始化测试
- `STM32F407VET_keil/` - STM32 完整工程示例
- `VS2022/` - Windows 平台测试工程

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 📮 联系方式

- **作者**: fool_cat
- **邮箱**: 2696652257@qq.com
- **GitHub**: [fool-cat/fc_embed](https://github.com/fool-cat/fc_embed)

## ⚠️ 注意事项

1. **线程安全**：大部分组件不提供内置锁机制，需由使用者保证线程安全
2. **FIFO 限制**：仅支持单生产者/单消费者场景
3. **配置优先**：使用前需根据实际需求创建 `fc_config.h`（基于 `fc_config_template.h`）
4. **内存对齐**：内存池要求内存按 `sizeof(size_t)` 对齐

## 🗺️ 路线图

- [x] 核心组件实现
- [x] CMSIS-Pack 支持
- [x] GitHub Actions CI/CD
- [ ] 更多平台移植示例
- [ ] 上位机工具（Transport 解析）
- [ ] 完整的 API 文档
- [ ] 性能基准测试

---

⭐ 如果这个项目对你有帮助，请给个 Star 支持一下！
