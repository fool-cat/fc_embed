---
name: version-bump
description: Use this skill whenever the user wants to bump, update, or change the version number of this repository, including updating version macros in component header files, the package version in fc_version.h, the PDSC release history, component Cversion attributes, or config file version attributes. Apply this skill when the user says things like "bump version", "release a new version", "update version to X.Y.Z", "add a release entry", "update the pdsc version", or "update component version". Always use this skill before editing any version macro or version field in `fool_cat.fc_embed.pdsc`.
---

# Version Bump

Apply this skill before changing any version number in this repository.

## Two-Level Version Model

This repository uses two levels of versioning:

| Level | Location | Controls |
|---|---|---|
| **Package** | `core/fc_version.h` — `FC_EMBED_VERSION_MAJOR/MINOR/PATCH` | `<release version="...">` in the PDSC |
| **Component** | Each component header — `FC_<NAME>_VERSION` | `<component Cversion="...">` in the PDSC |

In normal releases all component versions match the package version. They can diverge when only specific components change.

## Component Version Macro Map

Each component header carries its own version string macro. Read these macros to determine what `Cversion` to write into the PDSC.

| Header file | Version macro | PDSC component |
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
| `core/fc_config_template.h` | `FC_CONFIG_VERSION` | `<file attr="config" version="...">` on every component |

## Semantic Versioning Rules

Use [SemVer](https://semver.org/) — `MAJOR.MINOR.PATCH`:

- **PATCH** (`1.0.0 → 1.0.1`): bug fixes, doc corrections, no API change.
- **MINOR** (`1.0.0 → 1.1.0`): new component added, new API added, fully backward-compatible.
- **MAJOR** (`1.0.0 → 2.0.0`): breaking API change, component removed, incompatible behavior change.

When in doubt, ask the user which level applies before editing.

## Required Start Checks

```powershell
# Read all component version macros
Select-String -Path core/fc_auto_init.h, core/fc_fifo.h, core/fc_pool.h, `
    core/fc_port.h, core/fc_stdio.h, core/fc_log.h, core/fc_trans.h, `
    device/fc_type.h, device/fc_sig.h, device/fc_sig_filter.h, `
    device/fc_stp_base.h, device/fc_stp_curve.h, core/fc_config_template.h `
    -Pattern '_VERSION'
# Read package version
Get-Content core/fc_version.h | Select-String "FC_EMBED_VERSION_"
# Read current PDSC version fields
Select-String -Path fool_cat.fc_embed.pdsc -Pattern 'version='
# Today's date for the release entry
Get-Date -Format "yyyy-MM-dd"
# Confirm working tree is clean
git status --short
```

## Standard Workflow

### Full release (all components move together)

1. Update `FC_EMBED_VERSION_MAJOR/MINOR/PATCH` in `core/fc_version.h`.
2. Update **every** `FC_<NAME>_VERSION` macro in every component header to the new version string.
3. In `fool_cat.fc_embed.pdsc`:
   - Prepend a new `<release>` entry at the top of `<releases>`.
   - Set every `<component Cversion="...">` to the new version (read from the header macros).
   - Set every `<file attr="config" version="...">` to the new version (read from `FC_CONFIG_VERSION`).
4. Validate XML. Report diff.

### Partial release (only some components changed)

1. Update only the affected component header macros.
2. Update `FC_EMBED_VERSION_MAJOR/MINOR/PATCH` in `core/fc_version.h` (package version always bumps on any release).
3. In `fool_cat.fc_embed.pdsc`:
   - Prepend a new `<release>` entry.
   - Set each `<component Cversion="...">` to the value of its corresponding header macro (unchanged components keep their old version).
   - Update `<file attr="config" version="...">` only if `FC_CONFIG_VERSION` changed.
4. Validate XML. Report diff.

## PDSC Release Entry Guidelines

- Newest release goes **first** (top of `<releases>`).
- `date` must be `YYYY-MM-DD` format.
- Write a concise human-readable summary. Include:
  - Which components changed and why.
  - Any API additions or removals.
  - Any dependency changes.
- Do **not** delete old `<release>` entries — they are the public changelog.

## Config File Version

The `version` attribute on `<file attr="config" ...>` controls RTE config file migration in Keil MDK. Bump `FC_CONFIG_VERSION` in `core/fc_config_template.h` when the config template itself changes, then propagate to the PDSC.

## Pre-Commit Version Consistency Check

**Run this check before every `git commit` that touches any `core/` or `device/` file.**

Step 1 — Collect the current state:

```powershell
# All component version macros
Select-String -Path core/fc_auto_init.h, core/fc_fifo.h, core/fc_pool.h, `
    core/fc_port.h, core/fc_stdio.h, core/fc_log.h, core/fc_trans.h, `
    device/fc_type.h, device/fc_sig.h, device/fc_sig_filter.h, `
    device/fc_stp_base.h, device/fc_stp_curve.h, core/fc_config_template.h `
    -Pattern '_VERSION'
# Package version
Get-Content core/fc_version.h | Select-String "FC_EMBED_VERSION_"
# PDSC version fields
Select-String -Path fool_cat.fc_embed.pdsc -Pattern 'version='
# Files staged for commit
git diff --cached --name-only
```

Step 2 — Apply these rules:

| Condition | Required action before committing |
|---|---|
| A component header's `FC_<NAME>_VERSION` does not match its `Cversion` in the PDSC | Update the PDSC `Cversion` to match the header macro |
| `FC_EMBED_VERSION_*` in `core/fc_version.h` does not match the latest `<release version>` in the PDSC | Add a new `<release>` entry or update the package version macro |
| Any `<file attr="config" version="...">` does not match `FC_CONFIG_VERSION` | Update the PDSC config file version attributes |
| The staged files include component source/header changes but no version macro change | Ask the user whether a version bump is needed for this commit |

Step 3 — If any mismatch is found, fix it following the Standard Workflow above before proceeding with the commit.

Step 4 — Validate XML after any PDSC edit:

```powershell
python -c "import xml.etree.ElementTree as ET; ET.parse('fool_cat.fc_embed.pdsc'); print('XML OK')"
```

## Validation

```powershell
# XML syntax check
python -c "import xml.etree.ElementTree as ET; ET.parse('fool_cat.fc_embed.pdsc'); print('XML OK')"
# Confirm all version= values in PDSC
Select-String -Path fool_cat.fc_embed.pdsc -Pattern 'version='
```

## Final Report Checklist

State explicitly:

- the old version and the new version
- which component header macros were updated
- whether `FC_EMBED_VERSION_MAJOR/MINOR/PATCH` in `core/fc_version.h` was updated
- whether a new `<release>` entry was added with the correct date
- whether all affected `Cversion` attributes in the PDSC were updated (and which were left unchanged)
- whether `<file attr="config" version="...">` attributes were updated
- whether XML validation passed
