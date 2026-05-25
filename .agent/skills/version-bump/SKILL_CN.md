> 说明:
> - 实际触发、标准引用和后续维护基线只看 [SKILL.md](./SKILL.md)。
> - 本文件只是给中文开发者提供母语理解，不作为正式 skill 入口。

# Version Bump 中文版

在更新本仓库任何版本号之前，应先参考这个 skill。

## 两级版本模型

本仓库使用两级版本号:

| 级别 | 位置 | 控制范围 |
|---|---|---|
| **包级** | `core/fc_version.h` — `FC_EMBED_VERSION_MAJOR/MINOR/PATCH` | PDSC 中的 `<release version="...">` |
| **组件级** | 各组件头文件 — `FC_<NAME>_VERSION` | PDSC 中各 `<component Cversion="...">` |

正常发布时所有组件版本与包版本一致。只有部分组件变更时，可以单独升级对应组件的版本宏。

## 组件版本宏对照表

每个组件头文件都有自己的版本字符串宏。更新 PDSC 时，读取这些宏来确定对应的 `Cversion` 值。

| 头文件 | 版本宏 | PDSC 组件 |
|---|---|---|
| `core/fc_auto_init.h` | `FC_AUTO_INIT_VERSION` | `Cgroup="core" Csub="auto_init"` |
| `core/fc_fifo.h` | `FC_FIFO_VERSION` | `Cgroup="core" Csub="fifo"` |
| `core/fc_pool.h` | `FC_POOL_VERSION` | `Cgroup="core" Csub="pool"` |
| `core/fc_port.h` | `FC_PORT_VERSION` | `Cgroup="core" Csub="port"` |
| `core/fc_stdio.h` | `FC_STDIO_VERSION` | `Cgroup="core" Csub="stdio"` |
| `core/fc_log.h` | `FC_LOG_VERSION` | `Cgroup="core" Csub="log"` |
| `core/fc_trans.h` | `FC_TRANS_VERSION` | `Cgroup="core" Csub="trans"` |
| `device/fc_type.h` | `FC_TYPE_VERSION` | `Cgroup="device" Csub="type"` |
| `device/fc_sig.h` | `FC_SIG_VERSION` | `Cgroup="device" Csub="sig"` |
| `device/fc_sig_filter.h` | `FC_SIG_FILTER_VERSION` | `Cgroup="device" Csub="sig_filter"` |
| `device/fc_stp_base.h` | `FC_STP_BASE_VERSION` | `Cgroup="device" Csub="stp_base"` |
| `device/fc_stp_curve.h` | `FC_STP_CURVE_VERSION` | `Cgroup="device" Csub="stp_curve"` |
| `core/fc_config_template.h` | `FC_CONFIG_VERSION` | 每个组件的 `<file attr="config" version="...">` |

## 语义化版本规则

遵循 [SemVer](https://semver.org/) — `主版本.次版本.修订号`:

- **修订号** (`1.0.0 → 1.0.1`): 修复 bug、文档修正，无 API 变更。
- **次版本** (`1.0.0 → 1.1.0`): 新增组件、新增 API，完全向后兼容。
- **主版本** (`1.0.0 → 2.0.0`): 破坏性 API 变更、移除组件、不兼容的行为变更。

不确定时，先问用户应该升哪一级，再动手修改。

## 开始前必须检查

```powershell
# 读取所有组件版本宏
Select-String -Path core/fc_auto_init.h, core/fc_fifo.h, core/fc_pool.h, `
    core/fc_port.h, core/fc_stdio.h, core/fc_log.h, core/fc_trans.h, `
    device/fc_type.h, device/fc_sig.h, device/fc_sig_filter.h, `
    device/fc_stp_base.h, device/fc_stp_curve.h, core/fc_config_template.h `
    -Pattern '_VERSION'
# 读取包级版本
Get-Content core/fc_version.h | Select-String "FC_EMBED_VERSION_"
# 读取 PDSC 中的版本字段
Select-String -Path fool_cat.fc_embed.pdsc -Pattern 'version='
# 确认今天日期
Get-Date -Format "yyyy-MM-dd"
# 确认工作区干净
git status --short
```

## 标准流程

### 整包发布（所有组件一起升版本）

1. 修改 `core/fc_version.h` 中的 `FC_EMBED_VERSION_MAJOR/MINOR/PATCH`。
2. 修改**每个**组件头文件中的 `FC_<NAME>_VERSION` 为新版本字符串。
3. 修改 `fool_cat.fc_embed.pdsc`:
   - 在 `<releases>` 块**最上方**插入新的 `<release>` 条目。
   - 将每个 `<component Cversion="...">` 改为新版本（从头文件宏读取）。
   - 将每个 `<file attr="config" version="...">` 改为新版本（从 `FC_CONFIG_VERSION` 读取）。
4. 校验 XML，汇报 diff。

### 部分发布（只有部分组件变更）

1. 只修改受影响的组件头文件宏。
2. 修改 `core/fc_version.h` 中的包级版本（任何发布都必须升包版本）。
3. 修改 `fool_cat.fc_embed.pdsc`:
   - 插入新的 `<release>` 条目。
   - 每个 `<component Cversion="...">` 对应读取其头文件宏的值（未变更的组件保留旧版本）。
   - 只有 `FC_CONFIG_VERSION` 变了才更新 `<file attr="config" version="...">`。
4. 校验 XML，汇报 diff。

## release 条目写法

- 最新发布放在**最上面**。
- `date` 格式必须是 `YYYY-MM-DD`。
- 写简洁的人类可读摘要，包含:
  - 哪些组件变了、为什么变。
  - 新增或移除了哪些 API。
  - 依赖关系是否有变化。
- **不要删除**旧的 `<release>` 条目，它们是公开的变更日志。

## config 文件版本

`<file attr="config" version="...">` 控制 Keil MDK 的 RTE 配置文件迁移提示。当 `core/fc_config_template.h` 本身有变更时，升级 `FC_CONFIG_VERSION` 并同步到 PDSC。

## 提交前版本一致性检查

**每次 `git commit` 涉及 `core/` 或 `device/` 文件时，必须先执行此检查。**

第一步 — 收集当前状态:

```powershell
# 所有组件版本宏
Select-String -Path core/fc_auto_init.h, core/fc_fifo.h, core/fc_pool.h, `
    core/fc_port.h, core/fc_stdio.h, core/fc_log.h, core/fc_trans.h, `
    device/fc_type.h, device/fc_sig.h, device/fc_sig_filter.h, `
    device/fc_stp_base.h, device/fc_stp_curve.h, core/fc_config_template.h `
    -Pattern '_VERSION'
# 包级版本
Get-Content core/fc_version.h | Select-String "FC_EMBED_VERSION_"
# PDSC 版本字段
Select-String -Path fool_cat.fc_embed.pdsc -Pattern 'version='
# 本次暂存的文件
git diff --cached --name-only
```

第二步 — 按以下规则判断是否需要修复:

| 情况 | 提交前必须做的事 |
|---|---|
| 某组件头文件的 `FC_<NAME>_VERSION` 与 PDSC 中对应的 `Cversion` 不一致 | 将 PDSC 的 `Cversion` 更新为头文件宏的值 |
| `core/fc_version.h` 的 `FC_EMBED_VERSION_*` 与 PDSC 最新 `<release version>` 不一致 | 新增 `<release>` 条目或更新包级版本宏 |
| 任意 `<file attr="config" version="...">` 与 `FC_CONFIG_VERSION` 不一致 | 更新 PDSC 中的 config 文件版本属性 |
| 暂存文件包含组件源码/头文件变更，但没有对应的版本宏变更 | 询问用户本次提交是否需要升版本 |

第三步 — 发现不一致时，按上方"标准流程"修复后再提交。

第四步 — 修改 PDSC 后必须校验 XML:

```powershell
python -c "import xml.etree.ElementTree as ET; ET.parse('fool_cat.fc_embed.pdsc'); print('XML OK')"
```

## 校验命令

```powershell
# XML 语法校验
python -c "import xml.etree.ElementTree as ET; ET.parse('fool_cat.fc_embed.pdsc'); print('XML OK')"
# 确认 PDSC 中所有 version= 字段
Select-String -Path fool_cat.fc_embed.pdsc -Pattern 'version='
```

## 完成后汇报清单

明确说明:

- 旧版本号和新版本号各是什么
- 哪些组件头文件的版本宏被更新了
- `core/fc_version.h` 中的 `FC_EMBED_VERSION_MAJOR/MINOR/PATCH` 是否已更新
- 是否已在正确日期添加了新的 `<release>` 条目
- 哪些 `Cversion` 属性被更新了，哪些保持不变
- `<file attr="config" version="...">` 属性是否被更新
- XML 校验是否通过
