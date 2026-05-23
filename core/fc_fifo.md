# `fc_fifo` 设计与实现说明

本文档对应以下代码:

- [fc_fifo.h](./fc_fifo.h) — C 版本（字节流环形队列）
- [fc_fifo.hpp](./fc_fifo.hpp) — C++ 模板封装（固定容量泛型 FIFO）

---

# 第一部分：C 版本 — fc_fifo.h

## C-1. 模块定位

`fc_fifo` 是 `core/` 里最基础的字节流容器之一。

它提供的是一个:

- 固定容量
- 2 的幂大小
- 面向字节流
- 强调极致性能
- 适合单生产者 / 单消费者的环形队列

它不负责:

- 动态内存分配
- 多生产者 / 多消费者同步
- 协议解析
- 物理 IO

所以它更像一个"高性能基础缓存块"，上层的:

- [fc_port.h](./fc_port.h)
- [fc_trace.h](./fc_trace.h)
- `fc_fifo_vprintf`

都可以把它当作统一的字节缓存后端。

## C-2. 核心原理

`fc_fifo` 的核心思路很简单:

- `in` 表示累计写入计数
- `out` 表示累计读出计数
- 真实数组下标通过 `counter & mask` 得到
- `mask = size - 1`

整体关系如下:

```text
+-----------------------------------------------------------+
|                        上层模块                            |
|  port / trace / log / 自定义协议栈 / DMA 适配              |
+------------------------------+----------------------------+
                               |
                               v
+-----------------------------------------------------------+
|                         fc_fifo                            |
|  in / out / mask / pool                                    |
|  byte API / bulk API / linear zero-copy API                |
+------------------------------+----------------------------+
                               |
                               v
+-----------------------------------------------------------+
|                  用户提供的静态缓冲区 / 内存池              |
|                size 必须是 2^n, 例如 64/128/256            |
+-----------------------------------------------------------+
```

一个关键点是:

- `in` 和 `out` 不是环绕下标
- 它们是"单调递增计数器"

因此:

- 已用空间 = `in - out`
- 剩余空间 = `size - (in - out)`

这意味着它不需要像很多传统环形队列那样"浪费一个空槽位"区分满和空。

## C-3. 对象结构 `fc_fifo_t`

`fc_fifo_t` 可以拆成三部分理解。

### C-3.1 索引状态

- `in` — 累计写入位置
- `out` — 累计读取位置

这两个字段决定了队列当前的数据范围。

### C-3.2 存储配置

- `mask` — `size - 1`，用于把取模运算变成位与
- `pool` — 实际缓冲区地址

`fc_fifo` 要求缓冲区大小是 2 的幂，本质原因就是为了让:

```text
index % size
```

变成:

```text
index & (size - 1)
```

### C-3.3 线性窗口状态

- `linear_size_write`
- `linear_size_read`

这两个字段不是普通读写路径必须的，而是给"零拷贝线性窗口"接口服务的。

它们的语义是:

- 上一次 `linear_*_setup()` 暴露给外部的连续区长度
- 只要不为 0，就说明对应方向还处于"setup 但未 done"的忙状态

## C-4. API 参考

`fc_fifo` 的接口大致分成六类。

### C-4.1 初始化与复位

- `fc_fifo_init()`
- `fc_fifo_reset()`
- `fc_fifo_reset_read()`
- `fc_fifo_reset_write()`
- `fc_fifo_static_new_at()`

其中 `fc_fifo_static_new_at()` 很实用，它会在函数内部直接构造:

- 一个静态 `fc_fifo_t`
- 一个静态缓冲区

适合嵌入式里快速声明一个固定 FIFO。

但要注意:

- 这个宏内部用了静态对象
- 同一个指针变量通常只应构造一次

### C-4.2 状态查询

- `fc_fifo_get_size()`
- `fc_fifo_get_used()`
- `fc_fifo_get_free()`
- `fc_fifo_check_full()`
- `fc_fifo_check_empty()`

### C-4.3 单字节与批量读写

- `fc_fifo_write_byte()`
- `fc_fifo_read_byte()`
- `fc_fifo_peek_byte()`
- `fc_fifo_drop_byte()`
- `fc_fifo_write()`
- `fc_fifo_read()`
- `fc_fifo_peek()`
- `fc_fifo_drop()`

这是最常规的一组 API。

### C-4.4 覆盖写

- `fc_fifo_overwrite_byte()`
- `fc_fifo_overwrite()`

这组 API 的语义不是"写不进去就失败"，而是:

- 如果空间不够，就丢掉最旧的数据
- 尽量保留最新的数据

适合:

- 观测窗口
- 最近 N 字节历史
- 不关心旧数据、只关心最新状态的调试缓存

### C-4.5 线性零拷贝窗口

- `fc_fifo_linear_write_setup()`
- `fc_fifo_linear_write_done()`
- `fc_fifo_linear_read_setup()`
- `fc_fifo_linear_read_done()`
- `fc_fifo_linear_*_setup_limit()`
- `fc_fifo_linear_*_get_size()`
- `fc_fifo_linear_*_busy()`

这是 `fc_fifo` 和普通环形队列区别最大的地方。

它允许上层直接拿到"一段连续内存"，把数据直接写进去或直接读出去，而不是必须再走一次 `memcpy`。

### C-4.6 偏移读取与尾部丢弃

- `fc_fifo_seek_byte()`
- `fc_fifo_seek()`
- `fc_fifo_drop_tail_byte()`
- `fc_fifo_drop_tail()`

它们分别适合:

- 从当前读指针往后做只读查看
- 回退最新写入的数据

这对协议解析和命令行编辑都很有用。

## C-5. 普通读写路径

最常规的读写路径如下:

```text
write:
    free = size - used
    offset = in & mask
    先写尾部连续区
    再写头部回绕区
    in += 实际写入长度

read:
    used = in - out
    offset = out & mask
    先读尾部连续区
    再读头部回绕区
    out += 实际读取长度
```

因为写和读都只会最多分成"两段 memcpy"，所以:

- 逻辑简单
- 分支少
- 性能可预期

## C-6. 线性零拷贝路径

`linear_*` 接口是为下面这种场景准备的:

- DMA 发送
- DMA 接收
- 中断或驱动层要求传入连续缓冲区
- 避免中间复制

### C-6.1 发送方向示意

```text
上层先把数据写进 fifo
        |
        v
fc_fifo_linear_read_setup()
        |
        v
拿到一段连续可读内存 [ptr, size]
        |
        v
驱动 / DMA 直接发送这段内存
        |
        v
fc_fifo_linear_read_done(real_size)
```

### C-6.2 接收方向示意

```text
fc_fifo_linear_write_setup()
        |
        v
拿到一段连续可写内存 [ptr, size]
        |
        v
驱动 / DMA 直接把数据写进来
        |
        v
fc_fifo_linear_write_done(real_size)
```

### C-6.3 为什么要有 `*_limit()`

`fc_fifo_linear_*_setup_limit()` 会把单次连续窗口再限制到:

```text
buffer_size / (2^shift_n)
```

它的目的不是功能正确性，而是系统行为控制，例如:

- 一次发送不要太大，尽快释放部分缓冲区
- 一次接收不要占满过长时间，给上层更多处理机会

这也是 [fc_port.c](./fc_port.c) 里 `single_limit` 机制的基础。

## C-7. C 版本使用示例

```c
#include "fc_fifo.h"

// 方式一：手动初始化
uint8_t buffer[256];
fc_fifo_t fifo;
fc_fifo_init(&fifo, buffer, sizeof(buffer));

// 方式二：宏快速声明
fc_fifo_t *my_fifo = NULL;
fc_fifo_static_new_at(my_fifo, 8);  // 2^8 = 256 字节

// 写入
fc_fifo_write_byte(my_fifo, 0xAA);
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
fc_fifo_write(my_fifo, data, sizeof(data));

// 读取
uint8_t byte;
if (fc_fifo_read_byte(my_fifo, &byte)) {
    // byte = 0xAA
}

uint8_t buf[64];
size_t n = fc_fifo_read(my_fifo, buf, sizeof(buf));  // n = 4

// 状态查询
size_t used  = fc_fifo_get_used(my_fifo);
size_t free  = fc_fifo_get_free(my_fifo);
bool   empty = fc_fifo_check_empty(my_fifo);
bool   full  = fc_fifo_check_full(my_fifo);

// 零拷贝 DMA 发送
size_t send_size;
void  *send_ptr = fc_fifo_linear_read_setup(my_fifo, &send_size);
dma_start_send(send_ptr, send_size);  // 驱动层直接发送
// ... 等发送完成中断 ...
fc_fifo_linear_read_done(my_fifo, actual_sent);

// 覆盖写（不关心旧数据）
fc_fifo_overwrite_byte(my_fifo, 0xFF);

// 尾部丢弃（协议解析回退）
fc_fifo_drop_tail_byte(my_fifo);

// 偏移查看
uint8_t peek_val;
if (fc_fifo_seek_byte(my_fifo, &peek_val, 2)) {
    // peek_val = 当前队列第 3 个字节
}
```

---

# 第二部分：C++ 模板封装 — fc_fifo.hpp

## CPP-1. 概述与模板参数

[fc_fifo.hpp](./fc_fifo.hpp) 提供了一套泛型固定容量 FIFO 模板类 `fc_embed::fifo_t<T, log2_size>`，接口命名尽量贴近 STL。

```cpp
template <typename T, std::size_t log2_size>
class fifo_t;
```

| 模板参数 | 说明 |
| --- | --- |
| `T` | 存储的元素类型 |
| `log2_size` | 容量以 2 为底的对数，实际容量 `static_capacity = 1 << log2_size`，要求 `log2_size >= 1` |

内部使用 `std::aligned_storage` 做原始内存存储，通过 placement new 构造元素、显式析构销毁元素。索引计算与 C 版本一致：单调递增计数器 `in_` / `out_`，掩码 `mask_value = static_capacity - 1` 用于按位与取模。

## CPP-2. 类型定义

类内定义了标准 STL 风格类型别名：

| 别名 | 映射类型 |
| --- | --- |
| `value_type` | `T` |
| `size_type` | `std::size_t` |
| `difference_type` | `std::ptrdiff_t` |
| `reference` | `value_type &` |
| `const_reference` | `const value_type &` |
| `pointer` | `value_type *` |
| `const_pointer` | `const value_type *` |

## CPP-3. 构造与生命周期

| 接口 | 说明 |
| --- | --- |
| `fifo_t()` | 默认构造，空队列 |
| `fifo_t(std::initializer_list<value_type>)` | 从初始化列表构造，断言列表大小不超过容量 |
| `fifo_t(const fifo_t &)` | 拷贝构造，逐个 emplace 拷贝元素 |
| `fifo_t(fifo_t &&)` | 移动构造，逐个 emplace 移动元素（有条件 noexcept） |
| `operator=(const fifo_t &)` | 拷贝赋值，先 `clear()` 再 `copy_from()` |
| `operator=(fifo_t &&)` | 移动赋值，先 `clear()` 再 `move_from()`（有条件 noexcept） |
| `~fifo_t()` | 析构，调用 `clear()` 销毁所有元素 |

拷贝/移动赋值均带自赋值保护（`if (this != &other)`）。

## CPP-4. 容量与状态查询

| 接口 | 说明 |
| --- | --- |
| `capacity()` | 返回静态容量 `static_capacity`，`constexpr` |
| `max_size()` | 同 `capacity()`，`constexpr` |
| `size()` | 返回当前元素数量 `in_ - out_` |
| `free_size()` | 返回剩余可写空间 `capacity() - size()` |
| `empty()` | 判空，`in_ == out_` |
| `full()` | 判满，`size() == capacity()` |

## CPP-5. 重置操作

| 接口 | 说明 |
| --- | --- |
| `clear()` | 逐个 `pop()` 销毁所有元素 |
| `reset()` | 同 `clear()` |
| `reset_read()` | 同 `clear()`（当前实现） |
| `reset_write()` | 同 `clear()`（当前实现） |

> **注意**：`reset_read()` 和 `reset_write()` 当前实现与 `clear()` 完全一致（逐个 pop 销毁元素），未实现 C 版本"仅重置读写指针、不销毁数据"的语义。未来版本可能会优化为只重置 `in_` / `out_` 计数器。

## CPP-6. 写入（入队）

| 接口 | 说明 |
| --- | --- |
| `emplace(Args&&...)` | 原位构造元素；满时返回 `false` |
| `push(const T&)` | 拷贝写入，内部调用 `emplace` |
| `push(T&&)` | 移动写入，内部调用 `emplace` |
| `emplace_overwrite(Args&&...)` | 覆盖写入：满时先 `pop()` 丢弃最旧元素再 `emplace` |
| `push_overwrite(const T&)` | 覆盖拷贝写入 |
| `push_overwrite(T&&)` | 覆盖移动写入 |

`emplace` 使用 placement new 直接在存储槽位构造对象，避免中间临时对象的拷贝/移动开销。

## CPP-7. 读取（出队）

| 接口 | 说明 |
| --- | --- |
| `pop()` | 销毁队首元素；空时返回 `false` |
| `pop(T &value)` | 将队首元素移出到 `value` 后销毁；空时返回 `false` |
| `try_pop(T &value)` | 同 `pop(T&)` |
| `front()` | 返回队首引用（非 const / const 重载）；空时断言 |
| `back()` | 返回队尾引用（非 const / const 重载）；空时断言 |
| `operator[](size_type)` | 下标随机访问（非 const / const 重载）；越界断言 |
| `at(size_type)` | 下标随机访问，同 `operator[]`，越界断言 |
| `peek(T &value)` | 查看队首但不取出；空时返回 `false` |
| `seek(T &value, size_type offset)` | 查看指定偏移位置元素但不取出；越界返回 `false` |

## CPP-8. 批量操作

| 接口 | 说明 |
| --- | --- |
| `write(const T*, size_type)` | 批量写入，逐个 `push`，返回实际写入数量 |
| `overwrite(const T*, size_type)` | 批量覆盖写入；数据量超过容量时只保留最后 `capacity()` 个元素 |
| `peek(T*, size_type)` | 批量查看不取出，从队首开始 |
| `read(T*, size_type)` | 批量读取：先 `peek` 再 `drop` |
| `seek(T*, size_type, size_type)` | 从指定偏移开始批量查看不取出 |

`seek(data, count, offset)` 的偏移从 `out_ + offset` 开始，越界返回 0。

## CPP-9. 丢弃操作

| 接口 | 说明 |
| --- | --- |
| `drop(size_type)` | 从队首丢弃最多 `count` 个元素，返回实际丢弃数 |
| `drop_front()` | 丢弃队首一个元素，同 `pop()` |
| `pop_back()` | 丢弃队尾一个元素（`--in_` 并析构） |
| `drop_back(size_type)` | 从队尾丢弃最多 `count` 个元素，返回实际丢弃数 |

`pop_back()` 和 `drop_back()` 提供了 C 版本 `fc_fifo_drop_tail_byte` / `fc_fifo_drop_tail` 对应的尾部丢弃语义，适用于协议解析中回退最新写入的场景。

## CPP-10. C++ 使用示例

```cpp
#include "fc_fifo.hpp"

// 声明一个容量为 256 (2^8) 的 int 型 FIFO
fc_embed::fifo_t<int, 8> fifo;

// 初始化列表构造
fc_embed::fifo_t<int, 4> fifo2 = {1, 2, 3};  // 容量 16

// 写入
fifo.push(42);
fifo.emplace(100);               // 原位构造
fifo.write(some_array, 5);       // 批量写入

// 读取
int val;
if (fifo.pop(val)) {
    // val = 42
}
int front  = fifo.front();       // 查看队首（不取出）
int back   = fifo.back();        // 查看队尾
int second = fifo[1];            // 随机访问第 2 个元素
int third  = fifo.at(2);         // 带断言随机访问

// 覆盖写（不关心旧数据）
fifo.push_overwrite(999);        // 满时丢弃最旧的再写入
fifo.overwrite(sensor_data, 100); // 只保留最后 capacity() 个元素

// 尾部丢弃（协议解析回退）
fifo.pop_back();                 // 丢弃队尾一个元素

// 状态查询
bool   empty = fifo.empty();
size_t used  = fifo.size();
size_t cap   = fifo.capacity();   // 256
```

---

# 第三部分：版本对比与通用约束

## 3-1. C/C++ 版本差异对比

| 特性 | C 版 `fc_fifo.h` | C++ 版 `fc_fifo.hpp` |
| --- | --- | --- |
| 存储粒度 | 字节流 (`uint8_t`) | 泛型元素 (`T`) |
| 容量限制 | 2 的幂 | 2 的幂 (log2 指定) |
| 零拷贝窗口 | 支持 `linear_write/read_setup/done` | 不支持 |
| 批量读写 | 两段 `memcpy` 高性能路径 | 逐个元素 push/pop |
| 内存分配 | 用户提供 buffer 指针 | 内部 `aligned_storage` 静态数组 |
| 构造方式 | `fc_fifo_init()` | 构造函数 |
| 错误处理 | 返回值 `bool`/`size_t` | 返回值 + 断言 `fc_fifo_cpp_assert` |
| 尾部丢弃 | `fc_fifo_drop_tail()` | `pop_back()` / `drop_back()` |
| 随机访问 | `fc_fifo_seek()` | `operator[]` / `at()` |
| 代码膨胀 | 无（非模板） | 模板实例化会对不同 T/log2_size 生成独立代码 |

> **选择建议**：如果追求 DMA 零拷贝和极致字节流吞吐，选 C 版。如果需要类型安全和 STL 风格接口，选 C++ 版。在嵌入式场景中应谨慎控制 C++ 模板实例数量，避免固件体积膨胀。

## 3-2. 并发模型 (SPSC)

两个版本均明确偏向下面这个模型:

- 一个生产者只改写侧状态
- 一个消费者只改读侧状态

也就是经典 SPSC。

从源码注释看，C 版接口可以按"会改哪边索引"理解:

- **只改 `in` 的接口**：`write / write_byte / linear_write_done / drop_tail`
- **只改 `out` 的接口**：`read / read_byte / drop / linear_read_done`
- **同时可能改 `in/out` 的接口**：`reset / overwrite / overwrite_byte`

因此:

- 在单生产者 / 单消费者模型下，可做到无锁高性能
- 一旦出现多写者或多读者，就需要外部加锁

## 3-3. 使用约束

有几条约束两个版本都适用。

### 3-3.1 缓冲区大小必须是 2 的幂

否则 `mask` 优化不成立。C 版 `fc_fifo_init()` 会直接失败，C++ 版 `static_assert` 会在编译期检查。

### 3-3.2 `setup()` 到 `done()` 之间不要混用同方向操作（仅 C 版）

例如:

- `linear_write_setup()` 后，不要再调用普通 `write`
- `linear_read_setup()` 后，不要再调用普通 `read`

源码里也明确把这类接口列成了配套关系。

### 3-3.3 `busy` 只是状态提示，不是锁（仅 C 版）

`fc_fifo_linear_write_busy()` / `fc_fifo_linear_read_busy()` 只是根据 `linear_size_write` / `linear_size_read` 判断当前窗口是否未完成。它们不提供同步语义。

### 3-3.4 这是 FIFO 容器，不是消息队列

`fc_fifo` / `fifo_t` 自己不维护消息边界。

如果要存:

- 帧
- 包
- 命令

要由上层自己定义长度字段、分隔符或协议格式。

## 3-4. 在框架中的位置

在 `core/` 里，`fc_fifo` 大致处于下面的位置:

```text
+--------------------------------------------------------------+
|                        业务 / 协议 / 设备层                   |
|  shell / trace / port / log / 驱动适配                        |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                          fc_fifo                             |
|  字节缓存 / 回绕拷贝 / 线性窗口 / seek / overwrite            |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                     用户提供的原始内存区                       |
+--------------------------------------------------------------+
```

它不是"最终功能模块"，而是很多上层模块共用的基础设施。

## 3-5. 适用场景与建议

**适合的场景**:

- UART / USB / RTT / CAN 等收发缓存
- 调试输出缓存
- 协议栈字节流缓存
- DMA 零拷贝收发窗口（C 版）
- 泛型元素管理、STL 风格接口（C++ 版）

**不适合直接拿来做**:

- 多生产者队列
- 带消息优先级队列
- 需要动态扩容的缓存

**一句话总结**:

如果你的需求是 "极简、固定容量、追求速度"，那 `fc_fifo` 很合适。

如果你的需求是 "复杂同步语义、变长对象管理、强 ownership"，那应该在它之上再封一层，而不是直接往 `fc_fifo` 本体里加复杂逻辑。
