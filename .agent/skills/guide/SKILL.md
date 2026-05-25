---
name: guide
description: Apply this skill at the start of every task in the fc_embed repository. Use it to determine which other skills need to be loaded before proceeding. This is the entry-point routing skill — always apply it first when working on any file, feature, documentation, test, or build task in this repository, even if the user does not explicitly ask for it.
---

# fc_embed Skill Guide

Apply this skill before doing any work in this repository. Its sole job is to route the current task to the correct specialist skills.

## Step 1 — Classify the Task

Read the user's request and identify which of the following areas are touched. A single task may touch multiple areas.

| Area | Signals in the request |
| --- | --- |
| **Pack / Release** | `fool_cat.fc_embed.pdsc`, `gen_pack.sh`, `cmsis-pack/`, CMSIS-Pack, `.github/workflows/publish-pack.yml`, pack generation, release pipeline |
| **Version Bump** | version, bump, release version, `fc_version.h`, `FC_EMBED_VERSION`, `FC_*_VERSION`, `Cversion`, update version, 升版本, 发行版本 |
| **Host Test** | `test/host`, WSL, Linux test run, CMake host build, `ctest`, host-side validation, `build_linux`, `build_wsl` |
| **Documentation** | `readme.md`, `COMPONENTS.md`, `cmsis-pack/README.md`, `core/*.md`, `device/*.md`, `docs/records/`, module docs, repo overview |
| **Source Code Only** | `core/*.c`, `core/*.h`, `core/*.hpp`, `device/*.c`, `device/*.h`, no doc or pack files involved |
| **General Question** | Code review, explanation, analysis, no file edits planned |

## Step 2 — Load Required Skills

Based on the classification above, load the following skills **before** starting work:

| Classified Area | Skill to Load |
| --- | --- |
| Pack / Release | `cmsis-pack-guard` |
| Version Bump | `version-bump` |
| Host Test | `host-test-guard` |
| Documentation | `repo-doc-guard` |
| Source Code Only | *(no specialist skill required)* |
| General Question | *(no specialist skill required, unless the question is about pack baseline, test layout, or doc scope — then load the relevant skill)* |

If the task spans multiple areas, load all relevant skills before proceeding.

## Step 3 — Confirm and Proceed

After loading the required skills, follow their instructions. Do not skip the required start checks defined in each loaded skill.

If the task is purely a source-code change with no pack, test, or doc involvement, proceed directly without loading any specialist skill.

## Quick Reference — Repository Layout

```
fc_embed/
├── core/           # Core infrastructure modules (fifo, pool, port, log, stdio, trans, auto_init)
├── device/         # Device-layer modules (sig, sig_filter, stp_base, stp_curve, type)
├── test/host/      # Canonical host-side validation entry  →  host-test-guard
├── cmsis-pack/     # Generated pack artifacts              →  cmsis-pack-guard
├── .github/        # CI workflows                          →  cmsis-pack-guard (pack workflows)
├── fool_cat.fc_embed.pdsc                                  →  cmsis-pack-guard
├── gen_pack.sh                                             →  cmsis-pack-guard
├── readme.md                                               →  repo-doc-guard
├── COMPONENTS.md                                           →  repo-doc-guard
└── docs/records/                                           →  repo-doc-guard
```

## Available Skills in This Repository

| Skill Name | Responsibility |
| --- | --- |
| `cmsis-pack-guard` | Guard CMSIS-Pack definition, `pdsc`, `gen_pack.sh`, and pack-related workflows |
| `version-bump` | Manage version macros in component headers and synchronize them to the PDSC |
| `host-test-guard` | Constrain host-side validation to `test/host`, Linux native and WSL only |
| `repo-doc-guard` | Apply the correct baseline when editing repository or module documentation |
