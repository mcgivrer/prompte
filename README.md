# Bash Prompt Decorator (C / GCC)

Prompt bash riche et coloré écrit en C pur, compatible GCC et POSIX.

## Fonctionnalités

| Segment | Description |
|---------|-------------|
| **user@host** | Vert (normal) ou rouge (root). Indicateur SSH jaune. |
| **chemin** | Raccourci avec `~` pour le home. |
| **langage** | Détection automatique du projet (C, C++, Python, Rust, Go, Java, JS/TS, React, Vue, Angular, PHP, Ruby, Swift, Kotlin, Shell, Lua, Perl, Haskell, Elixir, Dart, Flutter, Docker, Make, CMake, Meson, Ninja…). |
| **git** | Branche courante, commits ahead/behind, fichiers modifiés `~N`, staged `+N`, untracked `?N`, stash `⚑N`. |
| **jobs** | Nombre de jobs en arrière-plan. |
| **load** | Alertes si charge CPU > 4.0. |
| **erreur** | Code de retour de la dernière commande en rouge. |

## Compilation

```bash
gcc -O2 -o prompt-decorator bash-prompt-decorator.c
sudo cp prompt-decorator /usr/local/bin/
```

## Installation dans Bash

### Méthode 1 — PromptCommand (recommandée)

Ajoutez dans votre `~/.bashrc` :

```bash
PROMPT_COMMAND='__update_prompt'

__update_prompt() {
    local last_exit=$?
    local jobs=$(jobs -p | wc -l)
    export JOBS="$jobs"
    export ?="$last_exit"
    PS1=$(/usr/local/bin/prompt-decorator 2>/dev/null || echo "\u@\h:\w\$ ")
}
```

Ou générez-le automatiquement :

```bash
./prompt-decorator --setup >> ~/.bashrc
```

### Méthode 2 — PS1 direct

```bash
PS1='$(prompt-decorator)'
```

### Mode simple (une ligne)

```bash
./prompt-decorator --simple
```

## Langages détectés

Le programme scanne le répertoire courant et identifie le langage par :
- **Fichiers de configuration** (`Cargo.toml`, `package.json`, `pyproject.toml`, `go.mod`, `CMakeLists.txt`, `Dockerfile`, etc.)
- **Extensions de fichiers source** (`.rs`, `.py`, `.cpp`, `.java`, `.tsx`, `.vue`, etc.)
- **Contenu** (détection React/Vue/Angular dans `package.json`)

## Architecture

- `PromptContext` — structure rassemblant toutes les infos du contexte
- `PromptBuilder` — builder de chaîne pour construire le prompt ANSI
- `detect_language()` — heuristique de détection du langage avec scoring par priorité
- `is_git_repository()` / `get_git_branch()` / `get_git_counts()` — introspection Git via `popen`
- `render_prompt()` — assemblage des segments colorés
