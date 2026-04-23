# `fc_fifo` 设计与实现说明

本文档对应以下代码:

- [fc_fifo.h](./fc_fifo.h)

## 1. 模块定位

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

所以它更像一个“高性能基础缓存块”，上层的:

- [fc_port.h](./fc_port.h)
- [fc_trace.h](./fc_trace.h)
- `fc_fifo_vprintf`

都可以把它当作统一的字节缓存后端。

## 2. 总体结构

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
- 它们是“单调递增计数器”

因此:

- 已用空间 = `in - out`
- 剩余空间 = `size - (in - out)`

这意味着它不需要像很多传统环形队列那样“浪费一个空槽位”区分满和空。

## 3. `fc_fifo_t` 对象结构

`fc_fifo_t` 可以拆成三部分理解。

### 3.1 索引状态

- `in`
  累计写入位置
- `out`
  累计读取位置

这两个字段决定了队列当前的数据范围。

### 3.2 存储配置

- `mask`
  `size - 1`，用于把取模运算变成位与
- `pool`
  实际缓冲区地址

`fc_fifo` 要求缓冲区大小是 2 的幂，本质原因就是为了让:

```text
index % size
```

变成:

```text
index & (size - 1)
```

### 3.3 线性窗口状态

- `linear_size_write`
- `linear_size_read`

这两个字段不是普通读写路径必须的，而是给“零拷贝线性窗口”接口服务的。

它们的语义是:

- 上一次 `linear_*_setup()` 暴露给外部的连续区长度
- 只要不为 0，就说明对应方向还处于“setup 但未 done”的忙状态

## 4. API 分组

`fc_fifo` 的接口大致分成六类。

### 4.1 初始化与复位

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

### 4.2 状态查询

- `fc_fifo_get_size()`
- `fc_fifo_get_used()`
- `fc_fifo_get_free()`
- `fc_fifo_check_full()`
- `fc_fifo_check_empty()`

### 4.3 单字节与批量读写

- `fc_fifo_write_byte()`
- `fc_fifo_read_byte()`
- `fc_fifo_peek_byte()`
- `fc_fifo_drop_byte()`
- `fc_fifo_write()`
- `fc_fifo_read()`
- `fc_fifo_peek()`
- `fc_fifo_drop()`

这是最常规的一组 API。

### 4.4 覆盖写

- `fc_fifo_overwrite_byte()`
- `fc_fifo_overwrite()`

这组 API 的语义不是“写不进去就失败”，而是:

- 如果空间不够，就丢掉最旧的数据
- 尽量保留最新的数据

适合:

- 观测窗口
- 最近 N 字节历史
- 不关心旧数据、只关心最新状态的调试缓存

### 4.5 线性零拷贝窗口

- `fc_fifo_linear_write_setup()`
- `fc_fifo_linear_write_done()`
- `fc_fifo_linear_read_setup()`
- `fc_fifo_linear_read_done()`
- `fc_fifo_linear_*_setup_limit()`
- `fc_fifo_linear_*_get_size()`
- `fc_fifo_linear_*_busy()`

这是 `fc_fifo` 和普通环形队列区别最大的地方。

它允许上层直接拿到“一段连续内存”，把数据直接写进去或直接读出去，而不是必须再走一次 `memcpy`。

### 4.6 偏移读取与尾部丢弃

- `fc_fifo_seek_byte()`
- `fc_fifo_seek()`
- `fc_fifo_drop_tail_byte()`
- `fc_fifo_drop_tail()`

它们分别适合:

- 从当前读指针往后做只读查看
- 回退最新写入的数据

这对协议解析和命令行编辑都很有用。

## 5. 普通读写路径

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

因为写和读都只会最多分成“两段 memcpy”，所以:

- 逻辑简单
- 分支少
- 性能可预期

## 6. 线性零拷贝路径

`linear_*` 接口是为下面这种场景准备的:

- DMA 发送
- DMA 接收
- 中断或驱动层要求传入连续缓冲区
- 避免中间复制

### 6.1 发送方向示意

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

### 6.2 接收方向示意

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

### 6.3 为什么要有 `*_limit()`

`fc_fifo_linear_*_setup_limit()` 会把单次连续窗口再限制到:

```text
buffer_size / (2^shift_n)
```

它的目的不是功能正确性，而是系统行为控制，例如:

- 一次发送不要太大，尽快释放部分缓冲区
- 一次接收不要占满过长时间，给上层更多处理机会

这也是 [fc_port.c](./fc_port.c) 里 `single_limit` 机制的基础。

## 7. 并发模型

`fc_fifo` 明确偏向下面这个模型:

- 一个生产者只改写侧状态
- 一个消费者只改读侧状态

也就是经典 SPSC。

从源码注释看，接口可以按“会改哪边索引”理解:

- 只改 `in` 的接口
  `write / write_byte / linear_write_done / drop_tail`
- 只改 `out` 的接口
  `read / read_byte / drop / linear_read_done`
- 同时可能改 `in/out` 的接口
  `reset / overwrite / overwrite_byte`

因此:

- 在单生产者 / 单消费者模型下，可做到无锁高性能
- 一旦出现多写者或多读者，就需要外部加锁

## 8. 使用约束

有几条约束非常重要。

### 8.1 缓冲区大小必须是 2 的幂

否则 `mask` 优化不成立，`fc_fifo_init()` 会直接失败。

### 8.2 `setup()` 到 `done()` 之间不要混用同方向操作

例如:

- `linear_write_setup()` 后，不要再调用普通 `write`
- `linear_read_setup()` 后，不要再调用普通 `read`

源码里也明确把这类接口列成了配套关系。

### 8.3 `busy` 只是状态提示，不是锁

`fc_fifo_linear_write_busy()` / `fc_fifo_linear_read_busy()` 只是根据:

- `linear_size_write`
- `linear_size_read`

判断当前窗口是否未完成。

它们不提供同步语义。

### 8.4 这是字节流 FIFO，不是消息队列

`fc_fifo` 自己不维护消息边界。

如果要存:

- 帧
- 包
- 命令

要由上层自己定义长度字段、分隔符或协议格式。

## 9. 在框架中的位置

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

它不是“最终功能模块”，而是很多上层模块共用的基础设施。

## 10. 适用场景与建议

适合:

- UART / USB / RTT / CAN 等收发缓存
- 调试输出缓存
- 协议栈字节流缓存
- DMA 零拷贝收发窗口

不适合直接拿来做:

- 多生产者队列
- 带消息优先级队列
- 需要动态扩容的缓存

如果你的需求是:

- “极简、固定容量、追求速度”

那 `fc_fifo` 很合适。

如果你的需求是:

- “复杂同步语义、变长对象管理、强 ownership”

那应该在它之上再封一层，而不是直接往 `fc_fifo` 本体里加复杂逻辑。
