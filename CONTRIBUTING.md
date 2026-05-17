# Contributing to avrOS

Thanks for your interest. This document captures the conventions for
branches, commits, and pull requests against this repository.

For *code* conventions see [doc/CODING_STANDARDS.md](doc/CODING_STANDARDS.md).
For architecture see [doc/SDD.md](doc/SDD.md).

## Branches

| Branch | Purpose |
|--------|---------|
| `main` | Tagged releases. Fast-forward only. |
| `develop` | Integration branch. Default target for PRs. |
| `feature/<short-name>` | Work in progress for a new feature. |
| `fix/<short-name>` | Bug fix branches. |
| `doc/<short-name>` | Documentation-only changes. |

Branch from `develop`, not `main`.

## Commit messages

Single-line subject + optional body. Subject ≤ 72 chars, imperative
mood, no trailing period.

```
<scope>: <subject>

<body — wrap at 80 chars, explain the why, not the what>

<trailers, e.g. Refs: #123>
```

`<scope>` is the directory or module touched, lowercase:

- `drv/uart`, `drv/gpio`, `drv/dac`, `drv/mem`, `drv/cpu`
- `sys/event`, `sys/queue`, `sys/fsm`, `sys/sys`, `sys/fio`
- `srv/cli`, `srv/log`, `srv/btn`, `srv/pcm`
- `app/<name>` for application changes
- `doc` for documentation
- `make` for build system
- `skill` for `.github/skills/...` changes

### Examples

```
sys/event: rework event arming to drop handler arg

The new event_t carries handlers via the descriptor instead of the
runtime status, so the same event can multiplex sub-types without
re-arming. Updates queue.c and fio.h to use the new evntWait API.

Refs: #42
```

```
doc: add coding standards document
```

```
make: pin Atmel.AVR-Dx DFP to 2.4.286
```

### Don't

- Don't prefix with ticket numbers — use a `Refs:` trailer.
- Don't write `WIP`, `fix typo`, or `temp` on commits that land in
  `develop`. Squash first.
- Don't mix mechanical reformatting with a substantive change. Land a
  `clang-format` reformat as a single dedicated commit.

## Pull requests

Title follows the same `<scope>: <subject>` convention as the merge
commit.

Description template:

```markdown
## What
One-sentence summary.

## Why
Background, design rationale, link to issue.

## How
High-level approach. Call out non-obvious decisions.

## Verification
- [ ] `make all` clean
- [ ] `make analyze` shows no new findings
- [ ] Tested on hardware: <board / target>
- [ ] CLI commands exercised: <list>
- [ ] `ram` / `rom` numbers (before → after) if size-relevant
```

## Merge strategy

- **Squash & merge** PRs into `develop`. Keeps history bisectable.
- **Merge commit** when merging `develop` → `main` at release time,
  with a descriptive tag.
- **Rebase** locally before opening the PR; do not rebase a PR branch
  after review has started.

## Tagging releases

Tags follow semver and live on `main`:

```
git tag -a v0.2.0 -m "avrOS v0.2.0"
git push origin v0.2.0
```

Update the root `VERSION` file in the same commit. `make version`
syncs `AVROS_VERSION` in `avrOSConfig.h`.

## Signed commits

Encouraged, not required:

```
git config commit.gpgsign true
git config user.signingkey <key-id>
```

## Code style enforcement

This repo ships an `.editorconfig` (universal) and a `.clang-format`
(optional reformat) at the root. Most editors honor `.editorconfig`
automatically. To check a file:

```bash
clang-format --dry-run -Werror path/to/file.c
```

Do not enable `formatOnSave` — drive-by reformatting in PRs hides the
substantive change.

## File headers

Every new `.c` / `.h` file must carry the BSD-style permission header
documented in [doc/CODING_STANDARDS.md](doc/CODING_STANDARDS.md).
The stamping script will generate it for you:

```bash
util/scripts/stamp_license.sh drv/foo.c "My new driver"
```
