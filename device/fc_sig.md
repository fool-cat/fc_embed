# `fc_sig` 设计与实现说明

本文档对应以下代码:

- [fc_sig.h](./fc_sig.h)
- [fc_sig.c](./fc_sig.c)

## 1. 模块定位

`fc_sig` 是一个面向二值状态的轻量抽象层。

它不关心底层对象到底是:

- GPIO 输入
- 限位开关
- 霍尔传感器
- 某个状态机的完成标志
- 经过阈值化后的模拟量结果

它只关心一件事:

- 当前逻辑状态是 `VALID`、`INVALID`，还是在配置触发器时使用的 `UNDEFINED`

因此 `fc_sig` 的核心职责不是“采样硬件”，而是把二值信号处理拆成三层:

1. 平台层负责提供原始状态
2. 滤波层负责把原始状态变成稳态
3. 事件层负责在稳态变化时触发回调

## 2. 总体分层

```text
+--------------------------------------------------------------+
|                       应用层 / 业务层                         |
|  fc_sig_trigger() / fc_sig_state() / 自定义 trigger 回调      |
+------------------------------+-------------------------------+
                               |
                               v
+--------------------------------------------------------------+
|                           fc_sig 核心                         |
|  fc_sig_heart()                                               |
|  - 调平台 ioctl 取原始值                                      |
|  - 调 filter_func 做稳态判定                                  |
|  - 基于 state_last / state_steady 做边沿触发                  |
+------------------------------+-------------------------------+
                               |
           +-------------------+-------------------+
           |                                       |
           v                                       v
+-----------------------------+     +----------------------------------+
|        平台采样适配层        |     |          滤波策略层               |
|  fc_sig_ioctl_t             |     |  fc_sig_filter_t                 |
|  FC_SIGNAL_IOCTL_GET_STATE  |     |  repeat / asym_repeat / ...     |
+-----------------------------+     +----------------------------------+
           |                                       |
           v                                       v
      GPIO / 传感器 /                      原始状态 -> 稳态
      软件状态源
```

可以把它理解成:

- `ioctl` 负责“我现在读到了什么”
- `filter_func` 负责“这个状态能不能被认定为稳定”
- `trigger` 负责“稳定状态发生了我关心的边沿变化后要做什么”

## 3. `fc_sig_t` 对象结构

`fc_sig_t` 本身就是整个框架的上下文对象，内部字段大致可分成四组:

### 3.1 平台绑定

- `user`
  由用户自行保存上下文，通常放 GPIO 端口、通道号、设备句柄等
- `ioctl`
  平台读取接口，目前最关键的是 `FC_SIGNAL_IOCTL_GET_STATE`

### 3.2 滤波绑定

- `filter`
  指向具体滤波器对象
- `filter_func`
  滤波函数入口

`fc_sig` 不管理滤波器内存，只做绑定和调用，因此:

- 不需要动态内存
- 不依赖具体滤波器类型
- 扩展新滤波器时通常不用改 `fc_sig` 主框架

### 3.3 触发状态

- `func_trigger`
  状态变化后要执行的回调
- `state_trigger`
  单边沿模式下，关心的目标状态
- `event`
  当前触发模式

### 3.4 状态缓存

- `state_steady`
  当前稳态，也就是对外可见状态
- `state_last`
  上一次稳态，用于边沿检测

一个很关键的点是:

- `state_last` 和 `state_steady` 都是“滤波后的状态”
- 因此触发逻辑针对的是“稳态边沿”，不是“原始抖动边沿”

这也是 `fc_sig` 适合做去抖/状态确认的根本原因。

## 4. 生命周期与调用路径

### 4.1 初始化: `fc_sig_init()`

初始化流程大致是:

```text
清零对象
-> 绑定 ioctl / user
-> 立即读取一次当前原始状态
-> 用这次读取结果初始化 state_steady 和 state_last
```

这里有两个设计意图:

1. 刚初始化时就让对象带着“真实初值”启动
2. 避免第一次进入 `heart()` 时因为 `last` 未定义而误判边沿

因此 `fc_sig` 初始化后就已经是一个可读状态，而不是“必须先跑几次心跳才有效”的对象。

### 4.2 绑定滤波器: `fc_sig_catch_filter()`

这个接口只做三件事:

1. 保存 `filter`
2. 保存 `filter_func`
3. 把 `state_steady` 回退到 `state_last`

第三点的意义是:

- 切换滤波器时，先以当前稳态作为新的起点
- 避免刚挂上滤波器就因为内部状态未建立而造成明显跳变

### 4.3 配置触发器: `fc_sig_trigger()`

触发器本质上是在配置一个“边沿订阅”。

支持两种模式:

- `state_trigger == FC_SIGNAL_STATE_UNDEFINED`
  进入双边沿模式，任意稳态跳变都触发
- `state_trigger == FC_SIGNAL_STATE_VALID/FC_SIGNAL_STATE_INVALID`
  进入单边沿模式，只有切换到指定稳态才触发

如果 `func_trigger == NULL`:

- 直接退回 `FREE`
- 表示不再监听事件

### 4.4 周期心跳: `fc_sig_heart()`

`fc_sig_heart()` 是整个框架的执行入口，正常使用时需要周期调用。

它的处理顺序是固定的:

```text
1. ioctl(GET_STATE) 读取当前原始状态 now_state
2. 如果存在 filter_func:
      state_steady = filter_func(filter, sig, now_state)
   否则:
      state_steady = now_state
3. 根据 event 模式检查 state_last -> state_steady 是否形成目标边沿
4. 如果满足条件则调用 func_trigger(sig)
5. 根据回调返回值决定是否继续保留触发器
6. state_last = state_steady
```

对应的处理链可以画成:

```text
raw state
   |
   v
ioctl(GET_STATE)
   |
   v
filter_func(optional)
   |
   v
state_steady
   |
   +--> 与 state_last 比较
           |
           +--> 满足边沿条件? ---- 否 ----> 结束本次心跳
           |
           +--> 是
                 |
                 v
             func_trigger(sig)
                 |
                 +--> END   : event = FREE
                 |
                 +--> AGAIN : 保持已注册状态
```

## 5. 触发状态机语义

`fc_sig` 的事件状态机很小，但语义很明确。

### 5.1 `FC_SIGNAL_EVENT_FREE`

自由态，不监听边沿，也不会触发回调。

适合:

- 只想读状态，不想做事件回调
- 某次触发完成后自动退订

### 5.2 `FC_SIGNAL_EVENT_EDGE_TRIGGER`

单边沿触发。

条件是同时满足:

1. `state_last != state_steady`
2. `state_steady == state_trigger`

也就是说:

- 必须真的发生跳变
- 而且必须跳到指定目标状态

例如:

- 只关心“无效 -> 有效”
- 或者只关心“有效 -> 无效”

### 5.3 `FC_SIGNAL_EVENT_EDGE_BOTH_TRIGGER`

双边沿触发。

条件只有一个:

- `state_last != state_steady`

因此只要稳态变化，就会触发回调。

适合:

- 想同时感知按下和松开
- 想在传感器进入/退出有效区时都收到事件

### 5.4 回调返回值的意义

`fc_sig_trigger_t` 返回的是 `fc_sig_trigger_ret_t`:

- `FC_SIGNAL_TRIGGER_RET_END`
  本次触发后退回 `FREE`
- `FC_SIGNAL_TRIGGER_RET_AGAIN`
  保持当前触发模式，后续边沿继续触发

因此 `fc_sig` 同时支持两类使用方式:

- 一次性触发
- 持续订阅式触发

## 6. 为什么说 `fc_sig` 是“调度器”，不是“滤波器”

`fc_sig` 自己并不实现具体去抖算法。

它只规定滤波器接口:

```c
typedef fc_sig_state_t (*fc_sig_filter_t)(void *filter, fc_sig_t *sig, fc_sig_state_t state);
```

这意味着任何滤波器只要满足:

1. 输入: 当前原始状态
2. 输出: 本次认定后的稳态

就能接入到 `fc_sig` 框架里。

当前仓库里已有的滤波器包括:

- `scale`
- `repeat`
- `asym_repeat`
- `integrator`
- `window`

每种算法的适用场景见 [fc_sig_filter.md](./fc_sig_filter.md)。

所以 `fc_sig` 的定位更准确地说是:

- 一个统一的二值信号处理骨架
- 一个把“采样、滤波、事件”串起来的调度器

## 7. 并发与原子区设计

`fc_sig.h` 里预留了四组宏:

- `SIGNAL_ATOMIC_ENTER/EXIT`
- `SIGNAL_HEART_ENTER/EXIT`

用途不同:

- `SIGNAL_ATOMIC_ENTER/EXIT`
  用在配置类接口，例如初始化、挂滤波器、设置触发器
- `SIGNAL_HEART_ENTER/EXIT`
  用在 `fc_sig_heart()` 周期执行路径

这样做的原因是:

- 有些平台上 `heart()` 可能在中断里运行
- 而配置接口可能在主线程/任务上下文里调用
- 如果两边会并发访问同一个 `fc_sig_t`，就需要平台自己补原子保护

如果项目里能保证:

- 同一个对象不会被并发访问

那么这些宏可以保留默认空实现。

## 8. 一个典型使用流程

下面是一个比较常见的“限位开关 + 连续重复去抖 + 上升沿触发”的接法:

```c
typedef struct
{
    fc_sig_t                sig;
    fc_sig_repeat_filter_t  repeat;
    gpio_port_t            *port;
    uint16_t                pin;
} limit_sw_t;

static fc_sig_state_t limit_ioctl(fc_sig_t *sig, fc_sig_ioctl_cmd_t cmd)
{
    limit_sw_t *self = (limit_sw_t *)sig->user;
    (void)cmd;

    return gpio_read(self->port, self->pin) ? FC_SIGNAL_STATE_VALID
                                            : FC_SIGNAL_STATE_INVALID;
}

static fc_sig_trigger_ret_t limit_trigger(fc_sig_t *sig)
{
    (void)sig;
    return FC_SIGNAL_TRIGGER_RET_AGAIN;
}

void limit_sw_init(limit_sw_t *self)
{
    fc_sig_init(&self->sig, limit_ioctl, self);

    fc_sig_filter_repeat_init(&self->repeat, 4);
    fc_sig_catch_filter(&self->sig, &self->repeat, fc_sig_filter_repeat);

    fc_sig_trigger(&self->sig, limit_trigger, FC_SIGNAL_STATE_VALID);
}

void limit_sw_task(limit_sw_t *self)
{
    fc_sig_heart(&self->sig);
}
```

这个例子里:

- 平台层只负责把 GPIO 电平映射成 `VALID/INVALID`
- `repeat` 负责去抖
- `trigger` 只在稳态真正进入有效后触发

## 9. 设计上的几个关键约束

### 9.1 回调应当非阻塞

源码注释已经明确:

- `func_trigger` 要求无阻塞

原因很简单:

- `fc_sig_heart()` 很可能在高频轮询或中断里跑
- 如果触发回调里做了阻塞动作，会直接破坏系统实时性

### 9.2 `ioctl` 需要返回“逻辑状态”，而不是原始电平

例如一个低电平有效的按键:

- 平台层应该在 `ioctl` 里把低电平映射成 `VALID`

而不是把“高低电平语义”继续暴露给上层。

这样上层永远只处理逻辑含义，不处理电平极性。

### 9.3 触发基于稳态，不基于毛刺

这是整个架构最有价值的一点。

只要:

- 原始状态先经过滤波

那么:

- `state_last != state_steady` 就代表“稳态真的发生了变化”
- 业务层拿到的是更干净、更可靠的事件

## 10. 适合放在整个项目的哪一层

从项目分层角度看，`fc_sig` 通常位于:

```text
业务状态机 / 设备逻辑
        |
        v
      fc_sig
        |
        +--> 平台读取函数(ioctl)
        |
        +--> 滤波器对象(filter)
        |
        +--> 触发回调(trigger)
```

因此它既不是最底层驱动，也不是最上层业务，而是一个很典型的“设备抽象中间层”。

它的价值在于:

- 把二值信号的处理方式统一起来
- 降低业务层对 GPIO 极性、抖动细节、回调细节的直接依赖

## 11. 一句话总结

`fc_sig` 的本质可以概括成一句话:

- 用统一对象把“原始二值状态读取、稳态判定、边沿回调”串起来

如果你只需要“读一个干净的二值状态”，它可以只启用 `ioctl + filter`。

如果你还需要“在状态切换时做动作”，它再额外挂上 `trigger` 即可。
