# Spécification — bash-prompt-decorator

## Résumé

Programme C single-file qui produit un prompt bash riche et coloré (compatible ANSI). Il détecte automatiquement le langage du projet courant, affiche l'état Git, et enrichit le prompt avec des informations contextuelles (user, host, SSH, charge CPU, jobs, code de retour).

**Langage :** C pur (GCC / POSIX, `_GNU_SOURCE`)
**Dépendances :** Aucune bibliothèque externe. Outils POSIX (`popen`, `getpwuid`, `gethostname`, `getloadavg`).

---

## Compilation

```bash
gcc -O2 -o prompt-decorator bash-prompt-decorator.c
```

Binaire produit : `prompt-decorator`

---

## Modes d'exécution

| Mode       | Commande                      | Description                                         |
| ---------- | ----------------------------- | --------------------------------------------------- |
| **Défaut** | `./prompt-decorator`          | Prompt multi-lignes riche avec segments colorés     |
| **Simple** | `./prompt-decorator --simple` | Prompt une ligne : `user@host:path (branch)$`       |
| **Setup**  | `./prompt-decorator --setup`  | Génère le snippet `PROMPT_COMMAND` pour `~/.bashrc` |

---

## Installation dans Bash

### Méthode 1 — `PROMPT_COMMAND` (recommandée)

Ajouter dans `~/.bashrc` :

```bash
PROMPT_COMMAND='__update_prompt'

__update_prompt() {
    local last_exit=$?
    local jobs=$(jobs -p | wc -l)
    export JOBS="$jobs"
    export LAST_EXIT="$last_exit"
    PS1=$(/usr/local/bin/prompt-decorator 2>/dev/null || echo "\u@\h:\w\$ ")
}
```

Généré automatiquement via :

```bash
./prompt-decorator --setup >> ~/.bashrc
```

### Méthode 2 — PS1 direct

```bash
PS1='$(prompt-decorator)'
```

> ⚠️ Cette méthode ne capture pas `LAST_EXIT` ni `JOBS` correctement.

---

## Segments du prompt

Le prompt par défaut est sur deux lignes.

### Ligne 1 — Informations contextuelles

| Segment       | Couleur fond            | Contenu                                                          | Condition d'affichage                      |
| ------------- | ----------------------- | ---------------------------------------------------------------- | ------------------------------------------ |
| **SSH**       | Jaune                   | `SSH`                                                            | Variable `SSH_CLIENT` ou `SSH_TTY` définie |
| **user@host** | Vert (ou rouge si root) | `user@host`                                                      | Toujours                                   |
| **Chemin**    | Bleu                    | Répertoire courant (raccourci `~`)                               | Toujours                                   |
| **Langage**   | Magenta                 | Icône + nom du langage détecté                                   | Langage ≠ inconnu                          |
| **Git**       | Cyan                    | Branche + statuts (ahead/behind/modified/staged/untracked/stash) | Répertoire Git                             |
| **Jobs**      | Jaune                   | `⚙ N`                                                            | Jobs en arrière-plan > 0                   |
| **Load**      | Rouge                   | `⚡ load`                                                        | Charge moyenne (1 min) > 4.0               |

### Ligne 2 — Prompt principal

| Élément                   | Couleur | Condition                                 |
| ------------------------- | ------- | ----------------------------------------- |
| `✗ N`                     | Rouge   | Dernière commande avec code de retour ≠ 0 |
| `$` (vert) ou `#` (rouge) | —       | Selon root ou non                         |

---

## Détection du langage

### Principe

Le répertoire courant est parcouru. Chaque fichier trouvé est comparé à une table de markers (fichiers de configuration ou extensions) associés à un **score de priorité**. Le langage avec le score le plus élevé est retenu.

### Priorités

| Catégorie         | Priorité | Exemples                                                                       |
| ----------------- | -------- | ------------------------------------------------------------------------------ |
| Config spécifique | 90–100   | `Cargo.toml` (100), `pyproject.toml` (95), `go.mod` (100), `package.json` (90) |
| Config secondaire | 80–85    | `requirements.txt` (80), `Pipfile` (85), `Dockerfile` (80)                     |
| Build system      | 70–90    | `CMakeLists.txt` (90), `Makefile` (70), `meson.build` (90)                     |
| Extensions source | 30–65    | `.rs` (50), `.py` (50), `.tsx` (65), `.vue` (60)                               |

### Langages supportés

C, C++, Python, Rust, Go, Java, JavaScript, TypeScript, Ruby, PHP, Swift, Kotlin, Shell, Lua, Perl, Haskell, Elixir, Dart, Flutter, React, Vue, Angular, Node.js, Docker, Make, CMake, Meson, Ninja

### Détection spéciale

Si `package.json` est détecté, le contenu est analysé pour distinguer React (`"react"`), Vue (`"vue"`) ou Angular (`"@angular"`).

---

## Détection Git

| Fonction              | Commande utilisée                                                                                      | Retour                                                 |
| --------------------- | ------------------------------------------------------------------------------------------------------ | ------------------------------------------------------ |
| `is_git_repository()` | `git rev-parse --is-inside-work-tree`                                                                  | `true` / `false`                                       |
| `get_git_branch()`    | `git symbolic-ref --short HEAD` → `git describe --tags --exact-match` → `git rev-parse --short HEAD`   | Nom de branche, tag, ou hash court                     |
| `get_git_counts()`    | `git rev-list --left-right --count`, `git diff --name-only`, `git ls-files --others`, `git stash list` | Nombres ahead/behind/modified/staged/untracked/stashed |

### Statuts Git affichés

| Symbole | Signification                     |
| ------- | --------------------------------- |
| `~N`    | N fichiers modifiés (jaune)       |
| `+N`    | N fichiers staged (vert)          |
| `?N`    | N fichiers untracked (rouge)      |
| `⚑N`    | N entrées dans le stash (magenta) |
| `↑N`    | N commits ahead (cyan)            |
| `↓N`    | N commits behind (cyan)           |

---

## Structures de données

### `PromptContext`

Rassemble toutes les informations du contexte courant : utilisateur, hostname, répertoire, branche Git, statuts Git, langage détecté, code de retour, type de session (root, SSH), jobs, load average.

### `PromptBuilder`

Buffer de 8192 octets avec position courante. Fournit des fonctions `pb_append`, `pb_segment`, `pb_raw` pour construire la chaîne ANSI du prompt.

### `LangInfo`

Table associant à chaque langage : nom, icône (nerd font), couleur前景, couleur background.

---

## Couleurs

Le programme définit des constantes ANSI pour 17 couleurs foreground, 6 couleurs background, le gras, et l'estompe. Les codes utilisent la syntaxe bash `\[\033[...m\]` pour être compatibles `PS1`.

---

## Contraintes et hypothèses

- **Buffer fixe :** `PromptBuilder.buffer` fait 8192 octets. Un prompt très long (chemin profond + beaucoup de statuts Git) pourrait théoriquement déborder.
- **Performance :** Chaque appel lance plusieurs `popen()` pour les commandes Git. En répertoire avec un historique Git lourd, cela peut ralentir le prompt.
- **Sécurité :** Les chemins contenant des quotes simples (`'`) dans `exec_cmd` peuvent casser les commandes shell construites via `snprintf`.
- **Icônes :** Les icônes supposent une police Nerd Font installée. Sans elle, les caractères s'affichent en box drawing ou ne s'affichent pas.
- **Dépendances Git :** La détection Git nécessite `git` installé et accessible dans le PATH.

---

## Fichiers du projet

```
bash-prompt-decorator.c   # Source unique (C)
install.sh                # Script d'installation (compile + copie + bashrc)
install-to-bash.sh        # Snippet PROMPT_COMMAND à ajouter dans ~/.bashrc
README.md                 # Documentation (FR)
docs/spec.md              # Ce fichier
```
