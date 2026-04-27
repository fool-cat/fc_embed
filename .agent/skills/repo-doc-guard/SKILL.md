---
name: repo-doc-guard
description: Use when modifying repository-level documentation, navigation pages, component overviews, module-adjacent docs, build/package docs or metadata, or doc maintenance records. Apply before editing files such as readme.md, COMPONENTS.md, cmsis-pack/README.md, fool_cat.fc_embed.pdsc, core/*.md, device/*.md, or docs/records notes. Keep root/build docs and package metadata aligned with the committed repository scope by default, update same-name core/device module docs from the latest source behavior, verify claims from real files, and avoid adding process-status notes to user-facing docs.
---

# Repo Doc Guard

Apply this skill before editing repository-level documentation.

## Scope

Treat these as formal docs unless the user explicitly says otherwise:

- `readme.md`
- `COMPONENTS.md`
- build/package-facing docs or metadata such as `cmsis-pack/README.md` and `fool_cat.fc_embed.pdsc`
- repo-level navigation or index pages
- module-adjacent docs such as same-name `core/*.md` and `device/*.md`

Treat these as process records:

- doc scope notes
- maintenance notes
- temporary maintenance records
- rationale that should not appear in formal user-facing docs

## Required start checks

Run these before changing formal docs:

```powershell
git status --short
git ls-tree -r --name-only HEAD
rg --files
```

Read the current formal doc and the source files it describes before editing.

For root overview docs, component overviews, and build/package-facing docs or metadata, inspect the committed file set they describe:

```powershell
Get-Content readme.md
Get-Content COMPONENTS.md
git show HEAD:readme.md
git show HEAD:COMPONENTS.md
rg -n "module-name|file-name|key-path" readme.md COMPONENTS.md cmsis-pack docs\records
```

For same-name module-adjacent docs under `core/` and `device/`, read the latest matching source/header files from the working tree before editing:

```powershell
Get-Content core\fc_pool.md
Get-Content core\fc_pool.c
Get-Content core\fc_pool.h
Get-Content device\fc_sig.md
Get-Content device\fc_sig.c
Get-Content device\fc_sig.h
```

Use `rg`, `git ls-tree`, and direct file reads to verify documentation claims against the correct baseline for the target doc.

## Core rules

1. Formal root overview docs, component overviews, and build/package-facing docs or metadata default to committed repository scope.
2. Same-name module-adjacent docs under `core/` and `device/` describe the latest current source behavior for that module.
3. Do not promote new or uncommitted modules into root/build docs unless the user explicitly asks for working-tree documentation.
4. Verify every module, API, path, and dependency claim against the relevant real files before writing it.
5. Do not infer formal module lists from IDE tabs alone; inspect the repository files.
6. Do not write process-status language into formal docs; describe the repository content directly.
7. If a temporary maintenance note is needed, put it under `docs/records/YYYY-MM-DD-<topic>.md`, but keep public overviews clean.
8. Do not add ad-hoc record files at repo root.
9. Do not mix implementation-process notes into formal overviews.

## Standard workflow

1. Classify the target file as formal doc or process record.
2. Choose the baseline:
   - `readme.md`, `COMPONENTS.md`, repo navigation, and build/package-facing docs or metadata use committed scope by default.
   - same-name `core/*.md` and `device/*.md` module docs use the latest matching source/header behavior.
3. Build the documented file/path set from `git ls-tree`, `rg --files`, and targeted source reads.
4. Read the current doc and remove references to:
   - wrong paths
   - deleted paths
   - obsolete behavior
   - process-status wording
5. If module source behavior changed, update the matching module doc to the latest behavior directly.
6. Self-check after editing:
   - key file references exist in the selected baseline
   - described APIs and behavior match the selected source files
   - formal docs contain no process-status notes

## Repository-specific guidance

This repo currently uses a mixed pattern:

- root-level overview docs
- module-adjacent `.md` files

Follow these conventions:

- `readme.md`, `COMPONENTS.md`, and build/package-facing docs or metadata should reflect the committed repository scope by default
- scope notes and maintenance records belong in `docs/records/`
- module-adjacent docs such as same-name `core/*.md` and `device/*.md` should describe the latest current module behavior without process caveats

## Recommended verification commands

```powershell
git status --short
git ls-tree -r --name-only HEAD
rg --files
rg -n "module-name|file-name|key-path" readme.md COMPONENTS.md cmsis-pack core device docs\records
```

## Final report checklist

State explicitly:

- which baseline was used for each formal doc family
- which source files or committed file lists were checked to verify the documentation
- whether process-status wording was kept out of formal docs
