---
name: repo-doc-guard
description: Use when modifying repository-level documentation, navigation pages, component overviews, or doc maintenance records. This skill must be applied before editing files such as readme.md, COMPONENTS.md, module index pages, or doc scope notes. It requires checking git HEAD file tree and worktree status first, then enforcing the rule that formal docs describe committed files by default while process records go into docs/records.
---

# Repo Doc Guard

Apply this skill before editing repository-level documentation.

## Scope

Treat these as formal docs unless the user explicitly says otherwise:

- `readme.md`
- `COMPONENTS.md`
- repo-level navigation or index pages
- module overview pages such as `core/*.md` and `device/*.md`

Treat these as process records:

- doc scope notes
- revision boundary notes
- temporary maintenance records
- explanations about uncommitted worktree content

## Required start checks

Run these before changing formal docs:

```powershell
git status --short
git ls-tree -r --name-only HEAD
```

If editing an existing formal doc, also inspect its committed baseline when relevant:

```powershell
git show HEAD:readme.md
git show HEAD:COMPONENTS.md
```

Use `rg` for targeted searches, but only treat paths that exist in `git ls-tree -r --name-only HEAD` as valid formal-doc candidates.

## Core rules

1. Formal docs default to `git HEAD` scope, not working-tree scope.
2. Do not add files, modules, or paths to formal docs if they do not exist in `HEAD`.
3. Do not infer formal module lists from open editor tabs or uncommitted files.
4. If the worktree contains uncommitted modules:
   - exclude them from formal docs by default
   - put any needed explanation in `docs/records/YYYY-MM-DD-<topic>.md`
5. Only include uncommitted content in formal docs if the user explicitly asks for working-tree documentation.
6. If uncommitted content is intentionally included, label that doc clearly as:
   - `based on working tree`
   - `includes uncommitted content`
7. Do not add ad-hoc record files at repo root.
8. Do not mix process notes into formal overviews.

## Standard workflow

1. Classify the target file as formal doc or process record.
2. Build the allowed file/path set from `git ls-tree -r --name-only HEAD`.
3. Read the current doc and remove references to:
   - uncommitted files
   - wrong paths
   - deleted paths
4. If the request touches uncommitted work:
   - keep formal docs on `HEAD`
   - write the boundary note under `docs/records/`
5. Self-check after editing:
   - key file references exist in `HEAD`
   - process records are under `docs/records/`

## Repository-specific guidance

This repo currently uses a mixed pattern:

- root-level overview docs
- module-adjacent `.md` files

Follow these conventions:

- `readme.md` and `COMPONENTS.md` should reflect committed repo state first
- scope notes and maintenance records belong in `docs/records/`

## Recommended verification commands

```powershell
git status --short
git ls-tree -r --name-only HEAD
rg -n "module-name|file-name|key-path" readme.md COMPONENTS.md docs\records
```

## Final report checklist

State explicitly:

- whether the doc baseline used `git HEAD`
- whether uncommitted files were found and excluded
- whether a `docs/records/...` note was added
