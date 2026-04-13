---
name: host-test-guard
description: Use when working on host-side validation, PC-side verification, WSL/Linux test runs, or any task touching `test/host` in this repository. Apply this skill before editing `test/host/README.md`, `test/host/CMakeLists.txt`, `test/host/support/*`, `test/host/cases/*`, or host verification configuration. Keep `test/host` as the canonical entry, prefer Linux native or WSL only, and do not revive `test/core` as the primary path.
---

# Host Test Guard

Apply this skill before editing or running host-side validation in this repository.

## Canonical Layout

- `test/host` is the only canonical host verification entry.
- `test/core` is legacy or historical context. Do not extend it or point new docs, scripts, or builds back to it unless the user explicitly asks for legacy cleanup.
- Keep only `build_linux` and `build_wsl` as supported host build directories.
- Keep harness code under `test/host/support/`.
- Keep host-side cases for `core/` modules under `test/host/cases/core/`.
- Keep host-side config in `test/host/fc_config.h`.

## Required Start Checks

Run these before changing host verification files:

```powershell
Get-ChildItem test\host
Get-Content test\host\README.md
Get-Content test\host\CMakeLists.txt
```

If the task changes harness code or test cases, also inspect:

```powershell
Get-ChildItem -Recurse -File test\host\support
Get-ChildItem -Recurse -File test\host\cases
```

If the task involves path migration or cleanup, inspect both old and new names:

```powershell
rg -n "test/core|test/host" test .agent
```

## Core Rules

1. Treat `test/host` as the source of truth for host-side validation.
2. Do not reintroduce `test/core/build*`, `build_local`, `build_ninja`, or 32-bit host validation flows.
3. Prefer Linux native or WSL verification only.
4. If the public host workflow changes, update `README.md`, `CMakeLists.txt`, and related support files together.
5. If a `core/` header depends on `device/` headers, keep host-side include paths aligned. Do not assume host validation is fully isolated from `device/`.
6. Do not rely on `git status` to detect `test/` changes in this repo. Inspect files directly because `test/` is ignored.
7. Preserve local `_deps/` when possible to avoid unnecessary online fetches during configure.
8. Keep build outputs under `test/host/build_linux` or `test/host/build_wsl`.
9. Keep generated runtime logs inside `test/host`. Do not allow host verification to create `test/log` or other log directories outside the canonical host path.
10. Keep `test/host/support/test_clock.*` usable in two modes: real clock by default for host-like behavior, and manual clock when deterministic scheduler tests need precise control.

## Standard Workflow

1. Read `test/host/README.md` and `test/host/CMakeLists.txt` to confirm the current baseline.
2. Edit only the canonical host path under `test/host`.
3. If adding a new host test:
   - put test cases in `test/host/cases/core/`
   - put shared harness code in `test/host/support/`
   - keep config changes in `test/host/fc_config.h` or `test/host/CMakeLists.txt`
4. If the task affects build behavior, keep the supported matrix to:
   - Linux native -> `build_linux`
   - WSL -> `build_wsl`
5. Verify with a real configure, build, or test run when feasible.

## Recommended Verification Commands

Linux shell:

```bash
cmake -S test/host -B test/host/build_linux
cmake --build test/host/build_linux --target fc_embed_tests -j2
ctest --test-dir test/host/build_linux --output-on-failure
```

PowerShell with WSL:

```powershell
wsl.exe -d Ubuntu-22.04 bash -lc "cd /mnt/<drive>/<repo-path> && cmake -S test/host -B test/host/build_wsl && cmake --build test/host/build_wsl --target fc_embed_tests -j2 && ctest --test-dir test/host/build_wsl --output-on-failure"
```

Resolve the repository path to its corresponding `/mnt/...` form before using the WSL command.

If configure unexpectedly falls back to online fetches, inspect whether `test/host/_deps/*-src` still contains a complete local source tree before assuming network access is required.

## Final Report Checklist

State explicitly:

- whether `test/host` was treated as the canonical path
- whether only `build_linux` and `build_wsl` were kept
- whether host verification was configured, built, or tested, and in which environment
- whether any legacy `test/core` references were intentionally left untouched
