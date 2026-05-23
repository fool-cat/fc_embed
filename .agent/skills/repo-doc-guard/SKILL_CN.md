# Repo Doc Guard 中文版

> 说明:
> - 本文件是 [SKILL.md](./SKILL.md) 的中文翻译辅助版本。
> - skill 的实际触发、标准引用和后续维护基线仍然以 `SKILL.md` 为准。
> - `SKILL_CN.md` 仅用于中文开发者阅读理解，不作为正式入口文件。

## 作用

在修改仓库级正式文档之前应用这个 skill。

## 适用范围

除非用户明确说明不是，否则以下内容视为正式文档：

- `readme.md`
- `COMPONENTS.md`
- `cmsis-pack/README.md`、`fool_cat.fc_embed.pdsc` 等构建/打包说明或元数据
- 仓库级导航页或索引页
- 模块旁路文档，例如与功能同名的 `core/*.md` 和 `device/*.md`

以下内容视为过程记录：

- 文档范围说明
- 维护说明
- 临时维护记录
- 不适合写入正式用户文档的过程原因说明

## 开始前必须执行的检查

在修改正式文档前，先执行：

```powershell
git status --short
git ls-tree -r --name-only HEAD
rg --files
```

修改已有正式文档时，先读取当前文档和它描述的源码。

修改根目录总览、组件总览、构建/打包说明或元数据时，先检查它们描述的已提交文件范围：

```powershell
Get-Content readme.md
Get-Content COMPONENTS.md
git show HEAD:readme.md
git show HEAD:COMPONENTS.md
rg -n "module-name|file-name|key-path" readme.md COMPONENTS.md cmsis-pack docs\records
```

修改 `core/` 和 `device/` 下与功能同名的模块旁路 `.md` 时，先读取工作区里最新的同名源码/头文件：

```powershell
Get-Content core\fc_pool.md
Get-Content core\fc_pool.c
Get-Content core\fc_pool.h
Get-Content device\fc_sig.md
Get-Content device\fc_sig.c
Get-Content device\fc_sig.h
```

用 `rg`、`git ls-tree` 和直接读文件来核对文档说法，并根据目标文档选择正确基线。

## 核心规则

1. 根目录总览、组件总览、构建/打包说明或元数据默认描述已提交仓库范围。
2. `core/` 和 `device/` 下与功能同名的模块旁路文档描述该模块当前最新源码行为。
3. 除非用户明确要求按 working tree 写文档，否则不要把新增或未提交模块提升到根目录/构建类文档里。
4. 文档里的模块、API、路径和依赖关系都要从对应真实文件核对。
5. 不要只根据 IDE 当前打开的标签页扩写正式模块列表。
6. 正式文档里不要写过程状态说明；直接描述仓库内容。
7. 如果确实需要临时维护记录，放到 `docs/records/YYYY-MM-DD-<topic>.md`，正式总览保持干净。
8. 不要在仓库根目录新增零散记录文件。
9. 不要把实现过程说明混进正式总览。

## 标准工作流

1. 先判断目标文件属于“正式文档”还是“过程记录”。
2. 选择基线：
   - `readme.md`、`COMPONENTS.md`、仓库导航、构建/打包说明或元数据默认使用已提交范围。
   - 与功能同名的 `core/*.md` 和 `device/*.md` 模块文档使用最新同名源码/头文件行为。
3. 根据 `git ls-tree`、`rg --files` 和定向源码阅读建立文档应覆盖的文件/路径集合。
4. 读取当前文档，清理其中对以下内容的引用：
   - 错误路径
   - 已删除路径
   - 过时行为
   - 过程状态说明
5. 如果模块源码行为变了，同名模块文档直接更新为最新行为。
6. 修改完成后自检：
   - 文档中提到的关键文件是否存在于所选基线
   - API 和行为说明是否匹配所选源码
   - 正式文档中是否没有过程状态说明

## 本仓库专用说明

这个仓库当前更接近下面这种结构：

- 根目录总览文档
- 模块旁路 `.md` 文档

因此建议遵循：

- `readme.md`、`COMPONENTS.md` 和构建/打包说明或元数据默认反映已提交仓库范围
- 范围说明和维护记录放在 `docs/records/`
- `core/*.md` 和 `device/*.md` 这类同名模块旁路文档直接描述当前模块最新行为，不写过程状态提示

## 推荐验证命令

```powershell
git status --short
git ls-tree -r --name-only HEAD
rg --files
rg -n "module-name|file-name|key-path" readme.md COMPONENTS.md cmsis-pack core device docs\records
```

## 最终汇报时应说明

- 各类正式文档分别使用了哪个基线
- 核对了哪些源码、文档文件或已提交文件列表
- 是否已避免在正式文档中写入过程状态说明
