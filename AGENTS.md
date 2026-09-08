# AGENTS.md

## What this is

Single-file C program (`bash-prompt-decorator.c`) that renders a colored bash prompt. Pure C, GCC, POSIX. No build system — just one `gcc` command.

## Build

```bash
gcc -O2 -o prompt-decorator bash-prompt-decorator.c
```

## Install (local dev)

```bash
./prompt-decorator --setup >> ~/.bashrc   # generates PROMPT_COMMAND snippet
```

## Install (system-wide)

```bash
sudo cp prompt-decorator /usr/local/bin/
```

## Repo gotchas

- `prompt-decorator` (compiled binary) and `2` (debug output capture) are tracked in git — likely should be in `.gitignore`.
- `install.sh` appends to `~/.bashrc` — don't re-run carelessly.
- No tests, no linter, no CI. Changes are verified by running the binary and inspecting the prompt visually.
- Requires `getloadavg()` — needs `_GNU_SOURCE` (already defined in the source).
- Git info is fetched via `popen()` shell commands — no `libgit2` dependency.
- README is in French; code comments are also in French. Keep consistent if editing.
