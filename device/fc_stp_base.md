# `fc_stp_base` 框架与实现说明

本文档对应以下代码:

- [fc_stp_base.h](./fc_stp_base.h)
- [fc_stp_base.c](./fc_stp_base.c)

## 1. 模块定位

`fc_stp_base` 是步进电机控制框架里的“基础执行层”。

它不直接实现:

- 具体加减速曲线数学
- GPIO/定时器寄存器操作
- 单位换算
- 原点回零、闭环反馈、上层业务流程

它负责的是这些中间职责:

1. 管理一次点到点运动的生命周期
2. 维护当前位置、目标位置、当前速度等核心状态
3. 按固定心跳节奏输出 step 脉冲边沿
4. 在每一步完成后调曲线层计算“下一步速度”
5. 把速度交给平台层换算成真实定时器频率

所以它是一个典型的“承上启下”的 base 层。

## 2. 在整个框架中的层次

从职责上看，`fc_stp_base` 处在下面这个位置:

```text
+--------------------------------------------------------------+
|                        应用层 / 设备逻辑                      |
|  move / stop / 任务调度 / 原点流程 / 业务状态机               |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                         fc_stp_base                           |
|  - 位置状态: s_now / s_target / s_last                        |
|  - 速度状态: v_now                                            |
|  - 脉冲相位: heart_index / pulse_scale                        |
|  - 生命周期: START / NEXT / DEC / END                         |
+-------------------+--------------------------+---------------+
                    |                          |
                    v                          v
+---------------------------+      +----------------------------------+
|        曲线计算层          |      |          平台驱动适配层           |
|  fc_curve_func_t          |      |  fc_stp_ioctl_t                  |
|  ladder / trapezoid /     |      |  fc_stp_freq_set_t               |
|  s_curve / custom         |      |  GPIO / Timer / Driver IC        |
+---------------------------+      +----------------------------------+
```

如果再往上叠一层，通常还会出现:

- 位置到物理量映射
- 零点/限位传感器处理
- 多轴协调
- 上位机命令接口

而这些都不应该直接塞进 `fc_stp_base`。

## 3. 为什么要把 base、curve、platform 拆开

这套拆分是整个设计最关键的地方。

### 3.1 `fc_stp_base` 只关心“运动骨架”

例如:

- 什么时候启动
- 什么时候输出 step 边沿
- 什么时候位置加 1 或减 1
- 什么时候请求下一步速度
- 什么时候结束

### 3.2 曲线层只关心“下一步速度是多少”

曲线层不需要自己发脉冲，也不直接操作定时器。

它只要回答:

- 启动时初速度是多少
- 下一步该快一点还是慢一点
- 如果缓停，现在还需要多少步才能停下

### 3.3 平台层只关心“怎么把命令落到硬件”

例如:

- `ENABLE` 对应哪根脚
- 正反转怎么切
- step 脉冲是拉高还是拉低
- 定时器重装值该怎么设置

这种拆法的好处是:

- 更换平台时不必动曲线算法
- 更换曲线算法时不必动底层驱动
- 上层只面对统一的移动接口

## 4. `fc_stp_base_t` 的对象结构

`fc_stp_base_t` 可以按下面四组理解。

### 4.1 平台接口

- `user`
  用户上下文
- `ioctl`
  负责方向、使能、step 边沿、开始结束等控制
- `freq_set`
  负责把逻辑速度 `v_now` 转换成底层实际输出频率

### 4.2 曲线接口

- `curve`
  曲线对象
- `curve_func`
  曲线调度入口

这两个字段让 `fc_stp_base` 对曲线类型完全无感知。

### 4.3 运动状态

- `v_now`
  当前逻辑速度
- `s_now`
  当前位置
- `s_target`
  目标位置
- `s_last`
  本次运动的起点，供曲线层计算已走距离
- `s_each_increase`
  每完成一个 step 后的位置增量，通常是 `+1` 或 `-1`

### 4.4 心跳与优化状态

- `pulse_scale`
  多少次 `heart()` 才完成一个完整 step 周期
- `heart_index`
  当前位于 step 周期的第几拍
- `curve_hold_steps`
  平台段优化计数，允许在速度不变的区间跳过重复曲线计算

## 5. 平台层接口分别承担什么职责

### 5.1 `fc_stp_ioctl_t`

`ioctl` 是离硬件最近的控制接口，对应 `fc_stp_ioctl_cmd_t`。

主要命令包括:

- `FC_STP_IOCTL_EDGE_ACTION`
  发出动作边沿
- `FC_STP_IOCTL_EDGE_FREE`
  释放动作边沿
- `FC_STP_IOCTL_DIR_POSITIVE`
  设置正方向
- `FC_STP_IOCTL_DIR_NEGATIVE`
  设置负方向
- `FC_STP_IOCTL_ENABLE`
  使能驱动
- `FC_STP_IOCTL_DISABLE`
  禁用驱动
- `FC_STP_IOCTL_START`
  标记一次运动开始
- `FC_STP_IOCTL_END`
  标记一次运动结束

这里有一个设计特点:

- `START/END` 不一定直接对应引脚
- 它们更像是给平台留下“开关定时器、切换状态、打日志”的钩子

### 5.2 `fc_stp_freq_set_t`

`freq_set` 用于把“逻辑速度”变成“底层频率配置”。

接口是:

```c
typedef void (*fc_stp_freq_set_t)(fc_stp_base_t *stp, int32_t freq, size_t *scale);
```

这意味着平台层可以同时做两类事情:

1. 直接改硬件定时器频率
2. 反向调整 `pulse_scale`

因此 `fc_stp_base` 并没有把“速度实现方式”写死成某一种。

## 6. 曲线层接口分别承担什么职责

曲线层统一通过 `fc_curve_func_t` 进入:

```c
typedef void (*fc_curve_func_t)(fc_stp_base_t *stp, fc_stp_calc_t calc_type, int32_t *v_next);
```

`calc_type` 代表 base 层在不同阶段对曲线层提出的不同请求。

### 6.1 `FC_STP_CLAC_START`

启动前调用。

曲线层通常在这里:

- 根据当前距离决定本次是梯形还是三角形
- 计算实际峰值速度
- 计算实际加速段长度
- 给出启动速度

### 6.2 `FC_STP_CLAC_NEXT`

每完成一个 step 后调用。

曲线层在这里给出:

- 下一步应使用的速度

### 6.3 `FC_STP_CLAC_DEC`

用户要求“安全停止”时调用。

曲线层在这里通常会:

- 根据当前速度反推刹车距离
- 把 `s_target` 改写到合适的减速终点

### 6.4 `FC_STP_CLAC_END`

一次运动完成后调用。

曲线层可以在这里:

- 清理临时状态
- 为下次运动复位内部变量

## 7. 初始化与绑定流程

### 7.1 `fc_stp_base_init()`

初始化时主要做三件事:

1. 清零整个对象
2. 绑定 `ioctl`、`freq_set`、`user`
3. 设置默认 `pulse_scale = 2`

默认 `pulse_scale = 2` 的含义很重要:

```text
第 1 次 heart: 发 EDGE_ACTION
第 2 次 heart: 发 EDGE_FREE
完成一个完整 step 周期
```

也就是说默认假设一个 step 由两个相位构成:

- 动作沿
- 释放沿

这更适合直接驱动 step/dir 一类器件。

如果平台把 step 脉宽已经交给其他硬件保证，也可以把 `pulse_scale` 设为 `1`。

这时语义会变成:

- 同一次 `heart()` 中发出 `EDGE_ACTION`
- 立即认为一个 step 周期完成
- 不再额外发 `EDGE_FREE`

### 7.2 `fc_stp_base_catch_curve()`

这个接口把 base 层和某条曲线绑定起来。

绑定后，base 层并不知道曲线具体是什么类型，只知道:

- 需要启动时调一次 `START`
- 每步调一次 `NEXT`
- 缓停时调一次 `DEC`
- 结束时调一次 `END`

这就是典型的“面向接口而不是面向实现”。

## 8. `heart()` 是整个运动引擎的核心

`fc_stp_base_heart()` 决定了这套框架的实际运行方式。

### 8.1 它不是“每次都走一步”

每次调用 `heart()` 时，先让 `heart_index++`。

然后根据 `heart_index` 所处相位决定要不要输出边沿:

```text
heart_index == 1 -> EDGE_ACTION
heart_index == 2 -> EDGE_FREE
heart_index >= pulse_scale -> 本 step 周期结束
```

这说明:

- `heart()` 是更细粒度的时基
- “一步走完”发生在 `heart_index` 累计满一个周期之后

### 8.2 位置更新发生在“完整 step 周期结束”时

当 `heart_index >= pulse_scale` 时:

1. `heart_index` 清零
2. `s_now += s_each_increase`
3. 进入下一步速度计算

因此 `s_now` 的含义不是“动作沿刚发出”，而是:

- 一个完整 step 已经完成后的逻辑位置

### 8.3 速度更新发生在“每一步之后”

当前位置更新后，如果尚未到达目标位置:

- 优先检查 `curve_hold_steps`
- 否则调用 `curve_func(FC_STP_CLAC_NEXT)`
- 再调用 `freq_set()`

这说明当前框架的本质是:

- 一个离散步进系统
- 速度按“每步一次”更新
- 而不是连续时间域实时微分方程控制

这也是 [fc_stp.md](./fc_stp.md) 会重点讨论各种离散曲线实现的原因。

## 9. 一次完整运动的时序

下面这张图基本对应 `move()` 到 `END` 的完整链路:

```text
fc_stp_base_move()
    |
    +--> 检查是否已停止(arrived)
    |
    +--> 计算 temp_target
    |
    +--> ENABLE
    |
    +--> 根据目标方向设置:
    |      DIR_POSITIVE / DIR_NEGATIVE
    |      s_each_increase = +1 / -1
    |
    +--> s_target = temp_target
    |
    +--> curve_hold_steps = 0
    |
    +--> curve_func(START, &v_now)
    |
    +--> freq_set(v_now, &pulse_scale)
    |
    +--> ioctl(START)
    |
    v
周期调用 fc_stp_base_heart()
    |
    +--> 发 EDGE_ACTION / EDGE_FREE
    |
    +--> 满一个周期后:
            s_now += s_each_increase
            |
            +--> 未到目标:
            |      curve NEXT 或 hold
            |      freq_set()
            |
            +--> 已到目标:
                   v_now = 0
                   curve_hold_steps = 0
                   curve_func(END)
                   freq_set(0)
                   ioctl(END)
```

## 10. `move()` 的实现逻辑

`fc_stp_base_move()` 的关键点有几个。

### 10.1 只能在停止后发起新的点到点运动

函数一开始就检查:

- `fc_stp_base_arrived(stp)`

如果还没到目标，就直接返回 `false`。

这说明当前 base 层默认模型是:

- 单次点到点运动
- 不支持在运行过程中直接插入新的目标

### 10.2 支持绝对移动和相对移动

- `FC_STP_MOVE_TO`
  直接把参数当目标位置
- `FC_STP_MOVE_RELATIVE`
  在当前目标基础上加偏移

注意当前实现里相对移动是基于 `s_target` 叠加，而不是基于 `s_now`。

这在“电机已停稳后再调用”时没有歧义，因为二者相等。

### 10.3 方向只在启动前设置一次

源码注释也明确了这一点:

- `fc_stp_base_heart()` 不负责改方向
- 方向只需要在启动之前改好

因此 base 层默认假设:

- 一次点到点运动期间方向不变

### 10.4 曲线启动和平台启动是分开的

`move()` 里先调:

- `curve_func(START, &v_now)`

再调:

- `freq_set(stp, v_now, &pulse_scale)`
- `ioctl(START)`

这说明曲线层先决定“以什么速度启动”，平台层再决定“如何按这个速度跑起来”。

## 11. `stop()` 为什么分安全停止和急停

`fc_stp_base_stop()` 提供了两种完全不同的终止语义。

### 11.1 `safe_stop = true`

表示缓停。

base 层会:

1. 清掉 `curve_hold_steps`
2. 调用 `curve_func(DEC, &v_now)`
3. 再调用 `freq_set()`

它不会立刻把 `s_target = s_now`。

它的思路是:

- 让曲线层重新规划一个减速终点
- 后续仍然通过正常 `heart()` 流程减速到停

### 11.2 `safe_stop = false`

表示急停。

base 层会直接:

1. `s_target = s_now`
2. `v_now = 0`
3. 调 `curve_func(END)`
4. 调 `freq_set(0)`
5. `heart_index = 0`
6. `curve_hold_steps = 0`
7. `ioctl(EDGE_FREE)`
8. 再次 `s_target = s_now`
9. `ioctl(END)`

这条路径本质上是:

- 立即终止当前运动状态机
- 尽快让底层恢复到空闲状态

适合:

- 故障保护
- 人工急停
- 不再关心减速过程是否平滑

## 12. `arrived()` 的判断条件为什么是“位置到达且速度为零”

`fc_stp_base_arrived()` 的判断是:

```text
s_now == s_target && v_now == 0
```

这比只比较位置更严谨。

因为在这套框架里:

- 即使位置已经相等，仍可能处于尚未完成结束清理的阶段
- `v_now == 0` 才代表本次运动真正结束

所以它表达的是:

- 不是“几何上接近目标”
- 而是“运动生命周期已经结束”

## 13. 位置修正接口的边界

### 13.1 `fc_stp_base_rectify_pos()`

只能在停止状态下使用。

它会同步修正:

- `s_now`
- `s_target`
- `s_last`

适合:

- 回零成功后把当前位置重新定义为 0
- 某次标定后重建逻辑坐标

### 13.2 `fc_stp_base_shift_pos()`

任何时候都能用，但源码注释已经写得很明确:

- 慎用

因为它会原子地整体平移:

- `s_now`
- `s_target`
- `s_last`

这类接口通常只适合:

- 上层回零逻辑
- 曲线/反馈模块内部校正

不适合随意暴露给普通业务流程。

## 14. 平台段跳算优化是怎么接入 base 层的

`fc_stp_base` 里新增的 `curve_hold_steps` 是一个很实用的性能优化点。

它的含义是:

- 后续还有多少步可以直接沿用当前速度
- 暂时不用重复计算 `curve NEXT`
- 暂时也不用重复调用 `freq_set`

在 `fc_stp_base_heart()` 里逻辑是:

```text
如果 curve_hold_steps > 0:
    curve_hold_steps--
否则:
    curve_func(NEXT)
    freq_set()
```

这个机制的好处是:

- 平台段很长时，可以显著减少重复运算
- base 层只提供通用“跳算槽位”
- 具体何时可以跳算，由曲线层决定

也就是说:

- 优化策略放在 curve 层
- 执行机制放在 base 层

这依然符合职责分离原则。

关于具体曲线如何设置 `curve_hold_steps`，见 [fc_stp.md](./fc_stp.md)。

## 15. 一张更细的运行时结构图

```text
                    +----------------------+
                    |  应用 / 调度器任务    |
                    |  move / stop / heart |
                    +----------+-----------+
                               |
                               v
        +---------------------------------------------------+
        |                  fc_stp_base_t                    |
        |                                                   |
        |  运动状态:  s_now / s_target / s_last / v_now     |
        |  方向状态:  s_each_increase                       |
        |  时基状态:  heart_index / pulse_scale             |
        |  优化状态:  curve_hold_steps                      |
        +-----------+--------------------------+------------+
                    |                          |
        calc_type   |                          | cmd/freq
                    v                          v
        +-------------------------+   +-------------------------+
        |      curve_func()       |   |        平台实现          |
        | START / NEXT / DEC / END|   | ioctl() / freq_set()    |
        +-------------------------+   +-------------------------+
                    |                          |
                    v                          v
         速度规划 / 刹车距离             step 引脚 / dir 引脚 /
         / 平台段跳算判断                enable 引脚 / timer
```

## 16. 平台移植时最需要明确的三件事

### 16.1 `heart()` 的调用时基

`fc_stp_base_heart()` 必须周期调用，而且这个周期要稳定。

因为:

- `pulse_scale` 是基于“heart 次数”在计数
- 如果 `heart()` 抖动很大，实际 step 时序也会被拉坏

### 16.2 `freq_set()` 到底调什么

项目里常见两种实现:

1. `heart()` 固定频率，`freq_set()` 主要改 `pulse_scale`
2. `heart()` 来自定时器中断，`freq_set()` 直接改定时器重装值

这两种方式都能兼容当前接口。

### 16.3 `EDGE_ACTION / EDGE_FREE` 的物理意义

要根据驱动芯片确定:

- 是拉高/拉低
- 还是置位/清位
- 是否需要固定脉宽

默认 `pulse_scale = 2` 就是为“动作沿 + 释放沿”这种模式准备的。

## 17. 一个最小接入示例

```c
typedef struct
{
    fc_stp_base_t    stp;
    trapezoid_curve_t curve;
    timer_handle_t   *tim;
    gpio_t            pin_step;
    gpio_t            pin_dir;
    gpio_t            pin_en;
} axis_t;

static void axis_ioctl(fc_stp_base_t *stp, fc_stp_ioctl_cmd_t cmd)
{
    axis_t *self = (axis_t *)stp->user;

    switch (cmd)
    {
    case FC_STP_IOCTL_EDGE_ACTION:  gpio_set(self->pin_step); break;
    case FC_STP_IOCTL_EDGE_FREE:    gpio_clr(self->pin_step); break;
    case FC_STP_IOCTL_DIR_POSITIVE: gpio_clr(self->pin_dir);  break;
    case FC_STP_IOCTL_DIR_NEGATIVE: gpio_set(self->pin_dir);  break;
    case FC_STP_IOCTL_ENABLE:       gpio_clr(self->pin_en);   break;
    case FC_STP_IOCTL_DISABLE:      gpio_set(self->pin_en);   break;
    case FC_STP_IOCTL_START:        timer_enable(self->tim);  break;
    case FC_STP_IOCTL_END:          timer_disable(self->tim); break;
    default: break;
    }
}

static void axis_freq_set(fc_stp_base_t *stp, int32_t freq, size_t *scale)
{
    axis_t *self = (axis_t *)stp->user;
    (void)scale;
    timer_set_step_freq(self->tim, freq);
}

void axis_init(axis_t *axis)
{
    fc_stp_base_init(&axis->stp, axis_ioctl, axis_freq_set, axis);

    trapezoid_curve_init(&axis->curve, 6000, 800, 1200);
    fc_stp_base_catch_curve(&axis->stp, &axis->curve, trapezoid_curve_func);
}

void axis_move_to(axis_t *axis, int32_t target)
{
    fc_stp_base_move(&axis->stp, FC_STP_MOVE_TO, target);
}

void axis_timer_isr(axis_t *axis)
{
    fc_stp_base_heart(&axis->stp);
}
```

这个例子正好体现了三层分工:

- `fc_stp_base` 管运动骨架
- `trapezoid_curve` 管速度规划
- `axis_ioctl/axis_freq_set` 管硬件落地

## 18. 并发与原子区边界

`fc_stp_base.h` 同样预留了四组宏:

- `STP_ATOMIC_ENTER/EXIT`
- `STP_HEART_ENTER/EXIT`

通常可以这样理解:

- `STP_ATOMIC_ENTER/EXIT`
  保护位置修正、状态改写等配置型接口
- `STP_HEART_ENTER/EXIT`
  保护 `fc_stp_base_heart()` 这类高频执行路径

为什么要这样留钩子:

- 很多项目里 `heart()` 在定时器中断里跑
- `move()`、`stop()`、`rectify_pos()` 可能在任务上下文里调用
- 如果这些路径会并发访问同一个 `fc_stp_base_t`，平台就必须自己补同步机制

如果项目里可以保证:

- 所有相关接口都在同一上下文串行调用

那么这些宏也可以保持默认实现。

## 19. 一句话总结

`fc_stp_base` 的本质可以概括成一句话:

- 它是一个把“点到点位置状态机、离散步进时基、曲线规划接口、底层脉冲驱动接口”粘合在一起的步进电机基础执行层

如果只看框架层次，可以把它理解成:

- 上面对应用暴露统一运动接口
- 下面同时对接曲线算法和硬件平台

这正是它在整个 `device` 层里的核心价值。
