---
name: cmsis-pack-guard
description: Use when generating or repairing the CMSIS-Pack for this repository, or before editing `fool_cat.fc_embed.pdsc`, `gen_pack.sh`, `cmsis-pack/*`, or pack-related GitHub Actions. Apply this skill when changing pack component layers, file inclusion scope, workflow triggers, or the release pipeline, and enforce the rule that the pack only describes tracked `HEAD` files unless the user explicitly asks for working-tree content.
---

# CMSIS Pack Guard

Apply this skill before changing the CMSIS-Pack definition or generation flow in this repository.

## Scope

Treat these as the canonical pack files:

- `fool_cat.fc_embed.pdsc`
- `gen_pack.sh`
- `cmsis-pack/README.md`
- `cmsis-pack/*.pack`
- `.github/workflows/cmake-single-platform.yml`
- `.github/workflows/publish-pack.yml`

Also inspect the tracked module tree under:

- `core/`
- `device/`

## Required Start Checks

Run these before editing pack files:

```powershell
git status --short
git ls-tree -r --name-only HEAD
Get-Content fool_cat.fc_embed.pdsc
Get-Content gen_pack.sh
Get-Content .github/workflows/cmake-single-platform.yml
Get-Content .github/workflows/publish-pack.yml
```

Use `rg` to confirm component names and dependencies from real code, not from memory:

```powershell
rg -n '#include "fc_[^"]+"' core device
```

## Core Rules

1. Treat the PDSC component list as a released pack contract, not a scratch view of the current worktree.
2. Only add modules to the pack when their files are tracked in `HEAD`.
3. Do not add ignored or untracked `device/` or `core/` files to `fool_cat.fc_embed.pdsc`.
4. When the repo contains uncommitted module work, keep the official pack scoped to tracked files unless the user explicitly asks for working-tree packaging.
5. Keep `fool_cat.fc_embed.pdsc`, `gen_pack.sh`, and pack-related workflows aligned in the same change.
6. If a new top-level directory is added to the pack, restrict the generated pack to tracked files. Do not rely on whole-directory copy alone.
7. Use CMSIS component layering with:
   - `Cgroup` for the major repository layer such as `core` or `device`
   - `Csub` for the concrete module such as `log`, `fifo`, or `sig`
8. Derive `<require ...>` relations from actual header or source dependencies before editing conditions.
9. Keep generated `.pack` artifacts under `cmsis-pack/`.
10. If a workflow commits generated pack files, make sure at least one trigger path can actually reach that commit step.

## Repository-Specific Baseline

As of the current tracked tree, the official pack baseline is:

- `core`: `fc_auto_init`, `fc_fifo`, `fc_pool`, `fc_port`, `fc_trans`, `fc_stdio`, `fc_log`
- `device`: `fc_type`, `fc_sig`, `fc_sig_filter`, `fc_stp_base`, `fc_stp_curve`

Files that exist only in the working tree must stay out of the official pack until they are committed.

## Standard Workflow

1. Inspect `git status --short` and `git ls-tree -r --name-only HEAD`.
2. Build the allowed pack component set from tracked `core/` and `device/` files only.
3. Update `fool_cat.fc_embed.pdsc` so component layering and conditions match the tracked code.
4. Update `gen_pack.sh` if pack directory scope or file pruning needs to change.
5. Review pack workflows and fix any trigger or commit path that became inconsistent with the generation flow.
6. Validate XML syntax and verify the pack scope when feasible.

## Recommended Verification Commands

PowerShell:

```powershell
git status --short
git ls-tree -r --name-only HEAD | rg "^(core|device|\\.github/workflows|cmsis-pack|gen_pack\\.sh|fool_cat\\.fc_embed\\.pdsc)"
python -c "import xml.etree.ElementTree as ET; ET.parse('fool_cat.fc_embed.pdsc')"
```

If a local Linux or WSL pack build is available:

```powershell
wsl.exe bash -lc "cd /mnt/d/GitHub_Clone/fc_embed && ./gen_pack.sh"
```

If a `.pack` file is regenerated, inspect its contents:

```powershell
Expand-Archive -Path cmsis-pack/fool_cat.fc_embed.1.0.0.pack -DestinationPath $env:TEMP\\fc_embed_pack_check -Force
Get-ChildItem -Recurse -File $env:TEMP\\fc_embed_pack_check
```

## Final Report Checklist

State explicitly:

- whether the pack scope was limited to tracked `HEAD` files
- whether untracked `core/` or `device/` files were excluded
- whether `fool_cat.fc_embed.pdsc`, `gen_pack.sh`, and workflows were kept in sync
- whether XML or pack generation was validated locally, and how
