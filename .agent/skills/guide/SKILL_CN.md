> 说明:
> - 实际触发、标准引用和后续维护基线只看 [SKILL.md](./SKILL.md)。
> - 本文件只是给中文开发者提供母语理解，不作为正式 skill 入口。

# fc_embed Skill 引导（中文版）

在这个仓库里开始任何工作之前，先应用这个 skill。它的唯一职责是：根据当前任务，决定需要加载哪些专项 skill，然后再开始执行。

## 第一步 — 判断任务类型

读取用户请求，识别涉及哪些领域。一个任务可能同时涉及多个领域。

| 领域 | 请求中的典型信号 |
| --- | --- |
| **打包 / 发布** | `fool_cat.fc_embed.pdsc`、`gen_pack.sh`、`cmsis-pack/`、CMSIS-Pack、`.github/workflows/publish-pack.yml`、pack 生成、发布流水线 |
| **版本号更新** | version、bump、release version、`fc_version.h`、`FC_EMBED_VERSION`、`FC_*_VERSION`、`Cversion`、update version、升版本、发行版本 |
| **主机端测试** | `test/host`、WSL、Linux 测试运行、CMake host 构建、`ctest`、主机端验证、`build_linux`、`build_wsl` |
| **文档** | `readme.md`、`COMPONENTS.md`、`cmsis-pack/README.md`、`core/*.md`、`device/*.md`、`docs/records/`、模块文档、仓库总览 |
| **纯源码修改** | `core/*.c`、`core/*.h`、`core/*.hpp`、`device/*.c`、`device/*.h`，不涉及文档或打包文件 |
| **纯问答** | 代码审阅、解释说明、分析，不计划编辑文件 |

## 第二步 — 加载所需 skill

根据上面的分类，在开始工作前加载对应的 skill：

| 分类领域 | 需要加载的 skill |
| --- | --- |
| 打包 / 发布 | `cmsis-pack-guard` |
| 版本号更新 | `version-bump` |
| 主机端测试 | `host-test-guard` |
| 文档 | `repo-doc-guard` |
| 纯源码修改 | *（不需要专项 skill）* |
| 纯问答 | *（通常不需要，但如果问题涉及 pack 基线、测试目录规范或文档基线，则加载对应 skill）* |

如果任务跨越多个领域，在开始执行前把所有相关 skill 都加载进来。

## 第三步 — 确认并执行

加载完所需 skill 后，按照各 skill 的指引执行。不要跳过各 skill 中定义的"开始前必须检查"步骤。

如果任务是纯源码修改，不涉及打包、测试或文档，直接执行，不需要加载任何专项 skill。

## 快速参考 — 仓库目录结构

```
fc_embed/
├── core/           # 核心基础模块（fifo、pool、port、log、stdio、trans、auto_init）
├── device/         # 设备层模块（sig、sig_filter、stp_base、stp_curve、type）
├── test/host/      # 主机端验证唯一正式入口  →  host-test-guard
├── cmsis-pack/     # 生成的 pack 产物        →  cmsis-pack-guard
├── .github/        # CI workflow             →  cmsis-pack-guard（pack 相关 workflow）
├── fool_cat.fc_embed.pdsc                    →  cmsis-pack-guard
├── gen_pack.sh                               →  cmsis-pack-guard
├── readme.md                                 →  repo-doc-guard
├── COMPONENTS.md                             →  repo-doc-guard
└── docs/records/                             →  repo-doc-guard
```

## 本仓库可用 skill 一览

| Skill 名称 | 职责 |
| --- | --- |
| `cmsis-pack-guard` | 守护 CMSIS-Pack 定义、`pdsc`、`gen_pack.sh` 和 pack 相关 workflow |
| `version-bump` | 管理组件头文件中的版本宏，并同步到 PDSC 的 `Cversion` 和 config 版本属性 |
| `host-test-guard` | 将主机端验证限定在 `test/host`，只支持 Linux 原生和 WSL |
| `repo-doc-guard` | 修改仓库文档或模块文档时选择正确的基线 |
