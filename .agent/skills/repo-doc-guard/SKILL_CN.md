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
- 仓库级导航页或索引页
- 模块总览页，例如 `core/*.md` 和 `device/*.md`

以下内容视为过程记录：

- 文档范围说明
- 修订边界说明
- 临时维护记录
- 对未提交工作区内容的解释说明

## 开始前必须执行的检查

在修改正式文档前，先执行：

```powershell
git status --short
git ls-tree -r --name-only HEAD
```

如果要修改已有正式文档，并且需要参考提交基线，再执行：

```powershell
git show HEAD:readme.md
git show HEAD:COMPONENTS.md
```

可以用 `rg` 做定向搜索，但正式文档中允许引用的路径，必须以 `git ls-tree -r --name-only HEAD` 中存在的路径为准。

## 核心规则

1. 正式文档默认以 `git HEAD` 为范围基线，而不是 working tree。
2. 如果某个文件、模块或路径不在 `HEAD` 中，就不要写进正式文档。
3. 不要根据 IDE 当前打开的标签页或未提交文件去扩写正式模块列表。
4. 如果工作区里存在未提交模块：
   - 默认从正式文档中排除
   - 如确需说明，单独写入 `docs/records/YYYY-MM-DD-<topic>.md`
5. 只有当用户明确要求“按 working tree / 未提交代码写文档”时，才可以把未提交内容纳入正式文档。
6. 如果确实把未提交内容写进正式文档，必须清楚标注：
   - `based on working tree`
   - `includes uncommitted content`
7. 不要在仓库根目录新增零散记录文件。
8. 不要把过程说明混进正式总览。

## 标准工作流

1. 先判断目标文件属于“正式文档”还是“过程记录”。
2. 先根据 `git ls-tree -r --name-only HEAD` 建立本次允许出现的文件/路径集合。
3. 读取当前文档，清理其中对以下内容的引用：
   - 未提交文件
   - 错误路径
   - 已删除路径
4. 如果这次需求涉及未提交工作：
   - 正式文档保持在 `HEAD` 基线
   - 边界说明写到 `docs/records/`
5. 修改完成后自检：
   - 文档中提到的关键文件是否存在于 `HEAD`
   - 过程记录是否放在 `docs/records/`

## 本仓库专用说明

这个仓库当前更接近下面这种结构：

- 根目录总览文档
- 模块旁路 `.md` 文档

因此建议遵循：

- `readme.md` 和 `COMPONENTS.md` 优先反映已提交仓库状态
- 范围说明和维护记录放在 `docs/records/`

## 推荐验证命令

```powershell
git status --short
git ls-tree -r --name-only HEAD
rg -n "module-name|file-name|key-path" readme.md COMPONENTS.md docs\records
```

## 最终汇报时应说明

- 本次文档是否以 `git HEAD` 为基线
- 是否发现并排除了未提交文件
- 是否新增了 `docs/records/...` 记录文件
