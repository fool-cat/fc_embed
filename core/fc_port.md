# `fc_port` 框架与实现说明

本文档对应以下代码:

- [fc_port.h](./fc_port.h)
- [fc_port.c](./fc_port.c)

## 1. 模块定位

`fc_port` 是 `core/` 里把“高速侧逻辑”和“低速侧物理 IO”连接起来的一层端口抽象。

它做的事情不是直接驱动 UART、USB 或 RTT，而是提供一个统一框架:

- 高速侧通过 FIFO 读写数据
- 低速侧通过 `phy` 回调和线性窗口对接
- 一个端口可以挂多个 ring buffer

所以它很像一个“带多缓冲区的 RTT/stream port 抽象”。

## 2. 在框架中的层次

```text
+--------------------------------------------------------------+
|                       应用 / 协议 / shell                     |
|  fc_printf / fc_write / fc_read / 自定义协议                  |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                           fc_port                            |
|  fc_port_write / read / trigger / end                        |
|  多 ring buffer / phy 回调 / 单次限流                        |
+-------------------+--------------------------+---------------+
                    |                          |
                    v                          v
+---------------------------+      +----------------------------------+
|         fc_fifo           |      |            物理层 phy             |
|  普通读写 / linear 窗口    |      |  UART DMA / USB / RTT / 自定义    |
+---------------------------+      +----------------------------------+
```

从职责划分上看:

- `fc_fifo` 只负责缓存
- `phy` 只负责真实慢速 IO
- `fc_port` 负责把两者拼起来

## 3. 方向语义

`fc_port_dir_t` 的方向定义是“从高速内核视角”看：

- `FC_PORT_DIR_OUT`
  高速侧写入 FIFO，慢速物理层把 FIFO 里的数据发出去
- `FC_PORT_DIR_IN`
  慢速物理层把数据收进 FIFO，高速侧再从 FIFO 里读

可以简单记成:

```text
OUT: core -> fifo -> phy
 IN: phy  -> fifo -> core
```

这也是 `fc_port_trigger()` / `fc_port_end()` 为什么会根据方向切换:

- `linear_read_setup`
- `linear_write_setup`

的原因。

## 4. `fc_port_t` 对象结构

`fc_port_t` 主要包含五组信息。

### 4.1 多缓冲区绑定

- `rb[PORT_RB_NUM]`
  每个索引对应一个 `fc_fifo_t`
- `rb_name[]`
  每个 FIFO 的名字，主要用于标识

### 4.2 物理接口

- `phy`
  物理 IO 回调

它的函数签名是:

- `fc_phy_io_t(size_t rb_index, void *buf, size_t len)`

约定是:

- 传给 `phy` 的 `buf ~ buf+len` 一定是连续内存

### 4.3 单次限流

- `rb_single_limit[]`

这个值的含义是:

- 单次 `trigger` 暴露给 `phy` 的最大窗口 = `fifo_size / 2^n`

它不影响 FIFO 总容量，只影响单次收发窗口大小。

### 4.4 方向

- `dir`

决定当前端口是:

- 输出型
- 输入型

### 4.5 RTT 辅助结构

源码还定义了 `fc_port_rtt_t`，保存:

- 标识字符串
- buffer 数量
- `port_in`
- `port_out`

这主要是给 RTT 风格的外部探测或调试工具定位端口对象。

## 5. API 分组

### 5.1 初始化与绑定

- `fc_port_init()`
- `fc_port_catch_fifo()`
- `fc_port_catch_phy()`
- `fc_port_static_alloc_rb()`

### 5.2 输出写入

- `fc_port_putc()`
- `fc_port_puts()`
- `fc_port_write()`
- `fc_port_printf()`
- `fc_port_vprintf()`

### 5.3 输入读取

- `fc_port_getc()`
- `fc_port_gets()`
- `fc_port_read()`
- `fc_port_peek()`

### 5.4 慢速 IO 握手

- `fc_port_trigger()`
- `fc_port_end()`

### 5.5 状态查询

- `fc_port_used()`
- `fc_port_free()`

### 5.6 默认标准输入输出

- `fc_default_port_init()`
- `fc_stdin`
- `fc_stdout`
- `fc_printf / fc_write / fc_read / fc_getchar` 等宏

## 6. 输出路径实现

输出端口最典型的工作流如下:

```text
应用层调用 fc_port_write / fc_port_printf
        |
        v
数据写入对应 rb fifo
        |
        v
fc_port_trigger()
        |
        v
fc_fifo_linear_read_setup[_limit]()
        |
        v
把连续可读区交给 phy(rb_index, buf, size)
        |
        v
底层发送完成后调用 fc_port_end(real_size)
        |
        v
fc_fifo_linear_read_done(real_size)
```

### 6.1 `fc_port_write()` / `fc_port_puts()` 的语义

这两组接口是偏“强一致写入”的:

- 先检查 FIFO 剩余空间是否足够
- 足够才整体写入
- 不足则触发丢失钩子

所以它们更接近:

- 要么全写进去
- 要么视为失败

### 6.2 `fc_port_printf()` 的语义

`fc_port_printf()` 走的是 `fc_fifo_vprintf()`。

这条链路底层是基于 `fc_vfprintf()` 和 FIFO 线性窗口推进的，因此当 FIFO 空间耗尽时:

- 会提前结束格式化写入
- 返回实际写入的字符数

所以它不像 `fc_port_write()` 那样天然是“整包全成或全失败”。

## 7. 输入路径实现

输入端口的工作流刚好相反:

```text
fc_port_trigger()
        |
        v
fc_fifo_linear_write_setup[_limit]()
        |
        v
给 phy 一段连续可写区
        |
        v
phy 把接收到的数据直接写入这段内存
        |
        v
fc_port_end(real_size)
        |
        v
fc_fifo_linear_write_done(real_size)
        |
        v
应用层用 fc_port_getc / read / peek 读取
```

这里 `fc_port` 的价值很明显:

- 收发两侧都复用同一套 FIFO 零拷贝机制
- 物理层不需要知道环形回绕细节

## 8. `trigger/end` 为什么拆成两步

如果直接设计成:

- `port -> phy -> port`

的一次同步调用，接口会很局促。

而拆成:

- `trigger()`
- `end(size)`

后，就可以自然支持:

- DMA
- 中断发送
- 中断接收
- 异步传输完成回调

`fc_port` 本身不关心 `phy` 是同步还是异步。

它只要求:

1. `trigger()` 之后拿到连续窗口
2. 传输结束时调用 `end(size)`

## 9. 阻塞与非阻塞行为

这块最好区分清楚。

### 9.1 阻塞式

- `fc_port_getc()`
- `fc_port_gets()`

`fc_port_getc()` 会在 FIFO 没数据时循环等待，并调用 `FC_WAIT_MOMENT()`。

也就是说:

- 是否真正“友好等待”
- 是否让出 CPU

取决于你怎么定义 `FC_WAIT_MOMENT()`。

默认是空操作。

### 9.2 非阻塞 / 尽量读写

- `fc_port_read()`
- `fc_port_peek()`
- `fc_port_used()`
- `fc_port_free()`

它们只是基于当前 FIFO 状态直接返回结果。

## 10. 线程安全与锁

`fc_port` 明确没有把锁写死在实现里。

它只暴露了两个可重定义宏:

- `FC_PORT_LOCK(port, rb_index, dir)`
- `FC_PORT_UNLOCK(port, rb_index, dir)`

默认它们都是空宏。

所以当前设计理念是:

- `fc_port` 自己不做锁策略假设
- 用户根据平台环境决定是否需要临界区 / mutex / irq lock

这和 [fc_fifo.h](./fc_fifo.h) 的风格是一致的，都是偏“高性能基础件”而不是“重策略框架”。

## 11. 丢失数据处理

默认实现提供了弱函数:

- `fc_port_lose_hook()`

它会分别给:

- `fc_stdout`
- `fc_stdin`

按 `rb_index` 统计累计丢失字节数。

当前会触发丢失记录的典型情况是:

- `fc_port_putc()` 单字节写失败
- `fc_port_puts()` / `fc_port_write()` 空间不足

你也可以在外部重写这个弱函数，把丢失数据接到:

- 统计模块
- 告警模块
- 调试接口

## 12. 默认标准输入输出

`fc_port.c` 提供了两个默认对象:

- `fc_stdout`
- `fc_stdin`

`fc_default_port_init()` 会做这些事情:

1. 初始化 `fc_stdout` 为 `OUT`
2. 初始化 `fc_stdin` 为 `IN`
3. 给两者各自分配一个静态 FIFO
4. 设置 RTT 风格标识结构 `fc_port_rtt`

随后通过 [fc_auto_init.h](./fc_auto_init.h) 把:

- `fc_default_port_init`

注册到 `ENV` 段，默认 order 是 `FC_PORT_INIT_ORDER`。

所以只要自动初始化链路打开，默认标准输入输出对象通常不需要你再手动构造。

## 13. 推荐使用方式

一个典型的 UART TX 适配模式如下:

```text
业务调用 fc_printf(...)
    -> 写入 fc_stdout 对应 fifo
    -> 调用 fc_out_trigger()
    -> UART/DMA 发送 setup 出来的连续区
    -> 发送完成中断里调用 fc_out_end(size)
```

一个典型的 UART RX 适配模式如下:

```text
空闲时调用 fc_in_trigger()
    -> 取到连续可写区
    -> UART/DMA 把数据写进来
    -> 接收完成中断里调用 fc_in_end(size)
    -> 上层通过 fc_read / fc_getchar 消费
```

## 14. 适用场景与边界

适合:

- UART / USB CDC / RTT / SWO / socket 这类字节流端口
- 需要多缓冲区索引的调试通道
- 需要把 DMA 和环形缓存拼起来的驱动层

不适合直接拿来做:

- 消息队列
- 带协议状态机的高层端口
- 自带复杂线程同步策略的框架

`fc_port` 的正确定位应该是:

- “FIFO 与慢速物理 IO 之间的适配层”

而不是:

- “完整通信协议框架”
