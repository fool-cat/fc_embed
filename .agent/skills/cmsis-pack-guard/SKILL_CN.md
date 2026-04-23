> 说明:
> - 实际触发、标准引用和后续维护基线只看 [SKILL.md](./SKILL.md)。
> - 本文件只是给中文开发者提供母语理解，不作为正式 skill 入口。

# CMSIS Pack Guard 中文版

在修改本仓库 CMSIS-Pack 定义或生成链路前，应先参考这个 skill。

## 适用范围

下面这些文件属于 pack 正式入口:

- `fool_cat.fc_embed.pdsc`
- `gen_pack.sh`
- `cmsis-pack/README.md`
- `cmsis-pack/*.pack`
- `.github/workflows/cmake-single-platform.yml`
- `.github/workflows/publish-pack.yml`

同时要核对真实模块树:

- `core/`
- `device/`

## 开始前必须检查

先执行:

```powershell
git status --short
git ls-tree -r --name-only HEAD
Get-Content fool_cat.fc_embed.pdsc
Get-Content gen_pack.sh
Get-Content .github/workflows/cmake-single-platform.yml
Get-Content .github/workflows/publish-pack.yml
```

并用真实代码确认依赖关系:

```powershell
rg -n '#include "fc_[^"]+"' core device
```

## 核心约束

1. `pdsc` 里的组件列表属于正式发布契约，不是工作区临时视图。
2. 只有 `HEAD` 已跟踪的模块，才能加入正式 pack。
3. 不要把未跟踪或被忽略的 `core/`、`device/` 文件加入 `fool_cat.fc_embed.pdsc`。
4. 如果仓库里有未提交模块，正式 pack 默认仍只描述已跟踪文件，除非用户明确要求基于 working tree 打包。
5. `fool_cat.fc_embed.pdsc`、`gen_pack.sh` 和 pack 相关 workflow 要一起维护。
6. 只要 pack 新增了顶层目录，就必须控制打包范围，只保留已跟踪文件，不能简单整目录照搬。
7. 分层时使用:
   - `Cgroup` 表示大层级，如 `core`、`device`
   - `Csub` 表示具体模块，如 `log`、`fifo`、`sig`
8. `<require ...>` 依赖关系要从真实头文件和源码推出来，不要凭印象写。
9. 生成的 `.pack` 文件统一落在 `cmsis-pack/`。
10. 如果 workflow 负责提交 pack 文件，要保证至少存在一条真的能走到提交步骤的触发路径。

## 当前仓库基线

当前 `HEAD` 已跟踪、适合作为正式 pack 基线的模块是:

- `core`: `fc_auto_init`、`fc_fifo`、`fc_pool`、`fc_port`、`fc_trans`、`fc_stdio`、`fc_log`
- `device`: `fc_type`、`fc_sig`、`fc_sig_filter`、`fc_stp_base`、`fc_stp_curve`

只存在于工作区、还没提交的模块，暂时不要写进正式 pack。

## 标准流程

1. 先看 `git status --short` 和 `git ls-tree -r --name-only HEAD`。
2. 只从已跟踪的 `core/`、`device/` 文件里建立 pack 允许集合。
3. 修改 `fool_cat.fc_embed.pdsc`，让组件分层和条件依赖对齐真实代码。
4. 如果 pack 顶层目录或打包范围变化，同步修改 `gen_pack.sh`。
5. 检查 pack workflow，修掉触发条件和提交路径不一致的问题。
6. 在可行时做 XML 或本地 pack 校验。

## 建议校验

```powershell
git status --short
git ls-tree -r --name-only HEAD | rg "^(core|device|\\.github/workflows|cmsis-pack|gen_pack\\.sh|fool_cat\\.fc_embed\\.pdsc)"
python -c "import xml.etree.ElementTree as ET; ET.parse('fool_cat.fc_embed.pdsc')"
```

如果本机能跑 Linux/WSL 打包:

```powershell
wsl.exe bash -lc "cd /mnt/d/GitHub_Clone/fc_embed && ./gen_pack.sh"
```

如果重新生成了 `.pack`，再解包检查实际内容:

```powershell
Expand-Archive -Path cmsis-pack/fool_cat.fc_embed.1.0.0.pack -DestinationPath $env:TEMP\\fc_embed_pack_check -Force
Get-ChildItem -Recurse -File $env:TEMP\\fc_embed_pack_check
```
