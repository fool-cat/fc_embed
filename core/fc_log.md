# `fc_log` 框架与实现说明

本文档对应以下代码:

- [fc_log.h](./fc_log.h)
- [fc_log.c](./fc_log.c)

## 1. 模块定位

`fc_log` 是 `core/` 里一套“宏层 + 运行时对象层 + 可插拔后端”的日志框架。

它不是简单地把 `printf` 换个名字，而是把日志这件事拆成了三层:

1. 宏层
   提供 `fc_log_error/info/debug/...` 这种易用接口
2. 序列化层
   通过 [fc_stdio.h](./fc_stdio.h) 的 `FC_FILE + fc_vfprintf()` 把日志格式化成字节流
3. 后端层
   通过可替换的 `alloc` / `write` 回调决定日志存到哪里、怎么发出去

因此 `fc_log` 的本质是:

- 一个可裁剪、可换后端、支持延迟输出的日志框架

## 2. 总体分层

```text
+--------------------------------------------------------------+
|                        业务 / 模块代码                        |
|  fc_log_error / fc_log_info / FC_LOG_MERGE / fc_log_write    |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                         fc_log 宏层                           |
|  文件级过滤 / 前缀拼接 / 作用域合并                           |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                        fc_log 运行时层                        |
|  fc_log_fprintf / fc_log_fwrite / fc_log_fflush              |
|  fc_log_t / fc_log_file_user_t / alloc / write               |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                fc_stdio + FC_FILE 序列化输出层                |
|                  fc_vfprintf / 分段写入 / swap                |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                    默认或自定义日志后端                       |
|  默认: fc_pool block chain -> fc_log_pool fifo               |
|  自定义: 直接发到 port / uart / 文件 / 网络                   |
+--------------------------------------------------------------+
```

这里最重要的一点是:

- `fc_log` 默认并不直接把字符串打印到 `stdout`
- 它默认是“先序列化、再缓冲、最后由外部统一取走”

## 3. `fc_log_t` 相关对象结构

### 3.1 `fc_log_mem_t`

描述一块当前正在使用的日志内存:

- `size`
- `buff`

### 3.2 `fc_log_file_user_t`

这是格式化过程中的中间上下文，包含:

- `log`
  指向根日志对象
- `mem`
  当前正在写的内存块
- `block_write`
  前面完整内存块累计写入长度
- `total_write`
  本次日志累计总长度
- `mem_chain`
  整个内存块链的首指针

它的作用不是“给业务层看”，而是把:

- 格式化状态
- 内存分配状态
- 多块日志链接关系

都串起来。

### 3.3 `fc_log_t`

`fc_log_t` 是对外的日志对象，包含:

- `alloc`
  日志内存分配策略
- `write`
  日志输出策略
- `file_user`
  merge 模式下复用的上下文
- `f`
  merge 模式下复用的 `FC_FILE`
- `level`
  当前运行时等级
- `merge`
  是否处于合并输出模式

所以 `fc_log_t` 不是“单纯配置对象”，它还具备承载 merge 运行时状态的能力。

但这里要区分两件事:

- `fc_log_t` 这个类型本身，确实带有 `f` / `file_user` / `merge` 这组运行时字段
- `FC_LOG_MERGE` 这个宏，默认并不是直接修改根日志对象，而是先复制一份 `FC_LOG_OBJ` 到作用域内临时对象，再让临时对象进入 merge

也就是说:

- “结构上支持 merge 状态驻留”是 `fc_log_t` 的能力
- “宏默认是否污染全局对象”则取决于 `FC_LOG_MERGE` 的展开方式

## 4. 宏层设计

头文件里最常用的宏 API 有:

- `fc_log_error()`
- `fc_log_warning()`
- `fc_log_info()`
- `fc_log_debug()`
- `fc_log_verbose()`
- `fc_log_printf()`
- `fc_log_write()`
- `fc_log_assert()`
- `FC_LOG_MERGE`

如果开启了 `FC_LOG_NOPREFIX_API`，还会导出无前缀别名:

- `log_error`
- `log_info`
- `LOG_MERGE`
- `log_write`

等。

## 5. 等级控制分成两层

`fc_log` 的等级控制不是只有一层。

### 5.1 文件级编译期过滤

通过:

- `FC_LOG_FILE_LEVEL`

控制。

这层会直接影响宏是否展开为真实代码。

例如某个 `.c` 文件如果把:

```c
#define FC_LOG_FILE_LEVEL FC_LOG_LEVEL_WARNING
```

放在 `#include "fc_log.h"` 之前，那么:

- `fc_log_debug()`
- `fc_log_verbose()`

就会在这一编译单元里被裁掉。

### 5.2 对象级运行时过滤

通过:

- `fc_log_set_level()`
- `fc_log_level()`

控制。

这层决定的是:

- 即使宏已经保留下来了
- 当前这个 `log` 对象要不要真正输出

所以更准确地说:

- `FC_LOG_FILE_LEVEL` 决定“代码能不能进来”
- `log->level` 决定“进来之后发不发”

## 6. 非 merge 模式的写入路径

默认情况下，普通一条日志的大致路径如下:

```text
fc_log_info(...)
    -> fc_log_fprintf()
    -> 在栈上创建临时 FC_FILE 和临时 file_user
    -> alloc(NEW) 拿第一块内存
    -> fc_vfprintf() 把日志写进去
    -> 如果当前块满了, 通过 FC_IO_SWAP 继续 alloc 下一块
    -> write(user) 处理整条日志的内存链
    -> alloc(FREE) 做收尾
```

这里有两个设计点很关键。

### 6.1 单次日志默认用栈临时对象

也就是说:

- 普通日志调用并不污染 `log` 对象本体里的 `FC_FILE`
- `log->f` 和 `log->file_user` 是给“当前进入 merge 的那个 `fc_log_t` 实例”复用
- 而 `FC_LOG_MERGE` 默认让这个实例成为作用域内的临时副本, 不是根对象本体

### 6.2 真正的大日志可以跨多块内存

因为 `fc_vfprintf()` 在窗口写满时会触发 `FC_IO_SWAP`，而 `__fc_log_alloc_write()` 会继续申请下一块内存。

所以:

- 单条日志不必被单个 `FC_LOG_LINE_SIZE` 限死
- 只是默认建议块大小是 `FC_LOG_LINE_SIZE`

## 7. merge 模式

`FC_LOG_MERGE` 的语义是:

- 让作用域内的多条日志先合并到同一条延迟输出链里
- 等作用域结束时再统一 flush

示意如下:

```text
FC_LOG_MERGE
{
    fc_log_info("A");
    fc_log_debug("B");
    fc_log_write(buf, len);
}
```

这时候:

- 这些输出不会每条都立即走一次 `write`
- 而是先复用“当前 merge 实例”的 `f` 和 `file_user`
- 最后由 `fc_log_fflush()` 一次提交

关键点在于，`FC_LOG_MERGE` 不是简单地把全局 `FC_LOG_OBJ.merge` 置为 `true`。

它在头文件里的实际形态等价于:

```c
fc_using(fc_log_t temp_log = FC_LOG_OBJ,
         *scope_log_ptr = &temp_log,
         scope_log_ptr->merge = true,
         fc_log_fflush(scope_log_ptr))
{
    ...
}
```

因此真实执行链路更接近:

```text
FC_LOG_OBJ(根对象)
    -> 拷贝出一份作用域临时 fc_log_t
    -> scope_log_ptr 指向这份临时对象
    -> 作用域内 fc_log_xxx 都写入临时对象的 f/file_user
    -> 离开作用域时 fc_log_fflush(临时对象)
```

这意味着:

- `FC_LOG_MERGE` 不会直接污染根对象上的 `log->f`
- `FC_LOG_MERGE` 不会直接污染根对象上的 `log->file_user`
- 根对象上的 `alloc` / `write` / `level` 等配置会被拷贝继承到临时对象

这里还有一个细节:

- 临时对象里的 `file_user.log` 也是从根对象拷贝来的
- 所以默认实现里，日志丢失统计这类“根对象归属”语义仍然可以落回原始日志对象

### 7.1 merge 的价值

适合:

- 把一组相关日志作为整体输出
- 减少后端写调用次数
- 保持日志块更完整

### 7.2 merge 的边界

merge 模式是否安全、是否适合跨线程共享，取决于:

- 你是否对同一个根 `fc_log_t` 做并发访问
- `alloc/write` 后端本身是否线程安全

`FC_LOG_MERGE` 的临时副本机制，解决的是“作用域内 merge 不直接污染根对象状态”这个问题。

但它并不自动解决:

- 多线程同时复制同一个根对象后的后端竞争
- 多个 merge 作用域同时写同一个 `fc_log_pool`
- 自定义 `alloc/write` 回调内部的并发安全问题

不过默认实现也不是完全没有并发保护。

`fc_log` 的默认后端是:

- `log_alloc_default()` 走 [fc_pool_alloc()](./fc_pool.c#L458)
- `log_write_default()` 走 [fc_pool_fifo_push()](./fc_pool.h#L145)
- 后续消费通常再走 [fc_pool_fifo_walk()](./fc_pool.c#L435)

而 `fc_pool` 在这些关键链表操作里，对:

- `list_free` 的摘链 / 回挂
- `fifo_used` 的 push / pop
- 部分 `realloc` 过程中的空闲链表提取

都使用了 `FC_ATOMIC_SCOPE` 包裹关键区，例如 [fc_pool_alloc()](./fc_pool.c#L467)、[fc_pool_free()](./fc_pool.c#L700)、[fc_header_fifo_push()](./fc_pool.c#L1220)、[fc_header_fifo_pop()](./fc_pool.c#L1258)。

`FC_ATOMIC_SCOPE` 的默认平台实现来自 [fc_arch.h](./fc_arch.h#L76)，在 MCU 默认移植下本质是:

- 进入临界区时关闭全局中断
- 离开临界区时恢复之前的中断状态

因此在“单核裸机 + 主循环/中断并发”这种典型单片机场景里，默认日志池至少具备一层基础原子保护，空闲链表和 FIFO 队列不会因为一次简单的抢占就被撕裂。

但这仍然不等于“整个日志框架天然线程安全”，因为下面这些部分并没有被一把统一大锁串起来:

- `fc_log_fprintf()` / `fc_log_fwrite()` 的完整格式化过程
- `FC_LOG_MERGE` 作用域内临时对象上的状态推进
- `fc_pool_walk()` / `fc_pool_fifo_walk()` 里用户提供的 `walker`
- 自定义 `alloc/write` 回调
- RTOS 多任务、SMP、多核等超出“关中断即可覆盖”的并发场景

所以更准确的结论是:

- 默认实现对底层池结构修改提供了 MCU 友好的原子保护
- 但如果你的系统存在任务级并发、跨核并发，或者自定义后端本身不是可重入的，仍然需要在更高层额外加锁或做串行化调度

## 8. `fc_log_fwrite()` 的定位

`fc_log_fprintf()` 是格式化文本日志。

`fc_log_fwrite()` 则更偏向“把一段原始字节流作为日志内容写进去”。

它和 `fprintf` 共享同一套后端模型:

- 都可能跨多块内存
- 都会走 `write`
- merge 模式下也会复用当前 merge 实例的 `f`
- 对 `FC_LOG_MERGE` 来说, 这个实例默认是作用域临时副本

因此:

- `fc_log_fprintf()` 适合结构化文本
- `fc_log_fwrite()` 适合原始缓冲、二进制片段或外部已格式化数据

## 9. 分配失败时怎么处理

这部分是 `fc_log` 比简单 `printf` 更完整的地方。

当日志内存分配失败时，框架会进入两种路径之一:

1. 直接终止本次序列化
2. 如果开启 `FC_LOG_POOL_FAIL_RECORD`
   就切到“只记录丢失长度”的失败回调

相关钩子包括:

- `FC_LOG_LOSE_HOOK`
- `fc_log_write_lose_hook()`

默认弱实现会记录默认日志对象累计丢失长度。

所以 `fc_log` 对失败不是“静默吞掉”，而是预留了专门的统计入口。

## 10. 默认后端实现

默认后端并不是直接 `fc_write()`。

它分成两部分。

### 10.1 `log_alloc_default()`

默认分配器使用:

- `fc_pool_alloc()`
- `fc_pool_link()`

从 `fc_log_pool` 里按块拿内存，并在需要时把多块链接起来。

### 10.2 `log_write_default()`

默认写函数会:

1. 标记最后一块真实使用了多少字节
2. 把整条日志对应的内存链压入 `fc_log_pool` 的 FIFO

注意:

- 这里不是直接发到串口
- 只是把日志放进“待输出队列”

头文件里也给了典型用法:

- 通过 `fc_pool_fifo_walk()` 遍历 `fc_log_pool`
- 在遍历回调里调用 `fc_write()`

因此默认路径其实是:

```text
fc_log_xxx()
    -> 格式化到 pool block
    -> push 到 fc_log_pool fifo
    -> 其他地方统一取走并输出
```

## 11. 默认日志对象与初始化

源码里提供了默认对象:

- `default_log`
- `fc_log_pool`

`fc_log_init()` 会初始化日志池，并通过:

- `INIT_EXPORT_ENV(fc_log_init, FC_LOG_INIT_ORDER)`

注册到自动初始化系统里。

所以在默认配置下:

- `fc_log_pool`
- `default_log`

通常会在环境初始化阶段准备好。

## 12. 与 `fc_stdio` / `fc_port` / `fc_pool` 的关系

三者关系可以这样理解:

### 12.1 对 `fc_stdio`

`fc_log` 借用的是它的格式化能力。

没有 `FC_FILE + fc_vfprintf()`，`fc_log` 就要自己重写一套格式化输出逻辑。

### 12.2 对 `fc_pool`

默认后端用 `fc_pool` 解决:

- 固定块分配
- 多块链接
- FIFO 化延迟输出

### 12.3 对 `fc_port`

`fc_log` 默认并不直接依赖 `fc_port` 才能工作。

但最常见的实际输出方式，通常还是:

- 最终把 `fc_log_pool` 里的内容走 `fc_write()` / `fc_port_write()` 发出去

所以它和 `fc_port` 的关系更像:

- 默认输出链路的下游消费者

## 13. 宏配置项的意义

`fc_log.h` 里有很多宏，比较关键的有:

- `FC_LOG_ENABLE`
  总开关
- `FC_LOG_FILE_LEVEL`
  当前编译单元允许的最大日志等级
- `FC_LOG_LINE_SIZE`
  单块日志建议大小
- `FC_LOG_USING_COLOR`
  是否输出 ANSI 颜色
- `FC_LOG_FMT_START`
- `FC_LOG_FMT_END`
- `FC_LOG_PREFIX_FMT`
- `FC_LOG_PREFIX_CONTENT`
- `FC_LOG_END`

这些宏的组合决定了:

- 前缀长什么样
- 是否带颜色
- 一条日志默认格式长什么样
- 后端块大小怎么取

因此 `fc_log` 的可配置性主要体现在“宏层格式”和“回调层后端”两端。

## 14. 推荐理解方式

如果只用一句话概括 `fc_log`，最准确的说法大概是:

- “一套基于 `FC_FILE` 的延迟序列化日志框架”

它不是:

- 直接往终端打印的最短路径

而是:

- 先把日志变成结构化字节流
- 再交给可切换的分配器和写后端

这也是它适合嵌入式复杂场景的原因。

## 15. 适用场景与边界

适合:

- 需要统一宏 API 的项目
- 需要编译期 / 运行时双层等级控制
- 需要日志延迟输出或异步转运
- 需要可替换内存策略和输出策略

不适合直接拿它当:

- “一调用就立刻串口 printf 完”的最简日志

如果你的需求只是极简同步打印，直接 `fc_printf()` 可能更轻。

如果你的需求是:

- 可筛选
- 可缓存
- 可换后端
- 可分阶段初始化

那 `fc_log` 的设计就很合适。

## 16. 使用时最重要的几个注意点

1. 默认写后端是把日志推进 `fc_log_pool`，不是直接输出到终端
2. `FC_LOG_FILE_LEVEL` 是编译期裁剪，`log->level` 是运行期过滤
3. `FC_LOG_MERGE` 会延迟 flush，且默认通过作用域临时 `fc_log_t` 合并输出，不会直接改根对象的 `f/file_user`
4. 是否线程安全取决于 `alloc/write` 回调及外部同步策略
5. 如果日志最终没人去消费 `fc_log_pool`，那日志就只是被缓存了，没有真正发出去
