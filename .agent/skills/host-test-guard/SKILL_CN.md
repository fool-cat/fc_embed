# Host Test Guard 中文版

> 说明:
> - 本文件是 [SKILL.md](./SKILL.md) 的中文翻译辅助版本。
> - skill 的实际触发、标准引用和后续维护基线仍然以 `SKILL.md` 为准。
> - `SKILL_CN.md` 仅用于中文开发者阅读理解，不作为正式入口文件。

## 作用

在这个仓库里进行 host 端验证、PC 端验证、WSL/Linux 测试运行，或者修改 `test/host` 相关内容前，先应用这个 skill。

## 规范入口

- `test/host` 是唯一正式的 host 端验证入口。
- `test/core` 只保留历史或迁移语境，不应再作为新的主路径继续扩写。
- host 端构建目录只保留 `build_linux` 和 `build_wsl`。
- 支撑代码放在 `test/host/support/`。
- `core/` 模块的 host 端用例放在 `test/host/cases/core/`。
- host 端配置放在 `test/host/fc_config.h`。

## 开始前必须检查

在修改 host 端验证相关文件前，先执行：

```powershell
Get-ChildItem test\host
Get-Content test\host\README.md
Get-Content test\host\CMakeLists.txt
```

如果要改测试支撑代码或用例，再执行：

```powershell
Get-ChildItem -Recurse -File test\host\support
Get-ChildItem -Recurse -File test\host\cases
```

如果任务涉及路径迁移或历史清理，再检查：

```powershell
rg -n "test/core|test/host" test .agent
```

## 核心规则

1. `test/host` 是 host 端验证的唯一基线目录。
2. 不要重新引入 `test/core/build*`、`build_local`、`build_ninja` 或 32 位 host 验证流程。
3. host 端验证只优先支持 Linux 原生和 WSL。
4. 如果 host 端公开工作流发生变化，要同步更新 `README.md`、`CMakeLists.txt` 和相关支撑文件。
5. 如果 `core/` 头文件依赖了 `device/` 头文件，host 端 CMake 也要同步保留对应 include 路径，不要假设 host 验证完全独立于 `device/`。
6. 不要依赖 `git status` 判断 `test/` 下是否有改动；这个仓库里 `test/` 被忽略了，要直接检查文件。
7. 尽量保留本地 `_deps/`，避免在 configure 阶段无谓回退到在线下载。
8. 构建输出只放在 `test/host/build_linux` 或 `test/host/build_wsl`。
9. 运行期日志也必须留在 `test/host` 内，不要让 host 验证流程再生成 `test/log` 这类跑出正式 host 路径之外的日志目录。
10. `test/host/support/test_clock.*` 应保持双模式能力：默认提供真实时钟以贴近宿主环境，需要严格控时的调度测试再切到手动时钟。

## 标准工作流

1. 先读 `test/host/README.md` 和 `test/host/CMakeLists.txt`，确认当前基线。
2. 只在 `test/host` 这条正式路径下修改。
3. 如果新增 host 端测试：
   - 测试用例放到 `test/host/cases/core/`
   - 通用支撑代码放到 `test/host/support/`
   - 配置改动放到 `test/host/fc_config.h` 或 `test/host/CMakeLists.txt`
4. 如果任务影响构建行为，只保留：
   - Linux 原生 -> `build_linux`
   - WSL -> `build_wsl`
5. 条件允许时，修改后必须做真实 configure、build 或 test 验证。

## 推荐验证命令

Linux shell:

```bash
cmake -S test/host -B test/host/build_linux
cmake --build test/host/build_linux --target fc_embed_tests -j2
ctest --test-dir test/host/build_linux --output-on-failure
```

PowerShell 下调用 WSL:

```powershell
wsl.exe -d Ubuntu-22.04 bash -lc "cd /mnt/<drive>/<repo-path> && cmake -S test/host -B test/host/build_wsl && cmake --build test/host/build_wsl --target fc_embed_tests -j2 && ctest --test-dir test/host/build_wsl --output-on-failure"
```

执行 WSL 命令前，先把仓库路径转换成对应的 `/mnt/...` 形式。

如果 configure 意外回退到在线下载，先检查 `test/host/_deps/*-src` 是否还是完整的本地源码树，不要先假设一定需要联网。

## 最终汇报时应说明

- 是否把 `test/host` 当作唯一正式路径处理
- 是否只保留了 `build_linux` 和 `build_wsl`
- 是否实际做了 host 端 configure、build 或 test 验证，以及在哪个环境执行
- 是否刻意保留了某些历史 `test/core` 引用未动
