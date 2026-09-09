# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Ce que c'est

Programme C single-file (`bash-prompt-decorator.c`) qui génère un prompt bash riche et coloré (ANSI), avec détection automatique du langage de projet et intégration Git. Pas de build system : une seule commande `gcc`.

## Commandes

```bash
# Compiler
gcc -O2 -o prompt-decorator bash-prompt-decorator.c

# Tester manuellement les différents modes
./prompt-decorator            # mode par défaut : ligne de statut en bas du terminal + prompt au-dessus
./prompt-decorator --simple   # prompt une ligne : user@host:path (branche)$
./prompt-decorator --status   # affiche uniquement la ligne de statut
./prompt-decorator --prompt   # affiche uniquement le symbole de prompt
./prompt-decorator --setup    # génère le snippet PROMPT_COMMAND pour ~/.bashrc

# Installer système-wide
sudo cp prompt-decorator /usr/local/bin/
cat install-to-bash.sh >> ~/.bashrc && source ~/.bashrc
```

Il n'y a **ni tests, ni linter, ni CI**. La seule vérification consiste à recompiler et à observer visuellement le prompt dans un vrai terminal (idéalement dans plusieurs contextes : dépôt Git avec des statuts variés, projets de langages différents, session SSH, utilisateur root).

## Architecture

Tout tient dans un seul fichier, organisé en sections délimitées par des commentaires `/* ==== ... ==== */` :

1. **Constantes de couleurs ANSI** — deux jeux parallèles : `COLOR_*`/`BG_*` (échappées `\[\033[...m\]`, pour usage dans `PS1`) et `RAW_*` (échappement direct `\033[...m`, pour affichage hors `PS1`, ex. la ligne de statut positionnée via séquences curseur). Ne pas mélanger les deux jeux : utiliser `RAW_*` pour tout ce qui est imprimé directement avec `printf`, `COLOR_*`/`BG_*` pour tout ce qui finit dans la variable `PS1`.

2. **`PromptContext`** — struct unique rassemblant tout l'état collecté pour un rendu : user/host/cwd, infos Git (branche, compteurs ahead/behind/modified/staged/untracked/stashed), langage détecté, code de retour, session root/SSH, jobs, load average. Rempli par `detect_context()`.

3. **`PromptBuilder`** — petit buffer-builder (`buffer[8192]` + position courante) avec `pb_append` (printf-like), `pb_segment` (segment coloré fond+texte), `pb_raw` (texte brut). Le buffer fixe de 8192 octets peut théoriquement déborder avec un chemin très profond combiné à beaucoup de statuts Git — vérifier `pb_append`/`snprintf` si on étend les segments.

4. **Détection de langage** (`detect_language`, table `LANG_TABLE`) — scanne le répertoire courant, associe fichiers de config (`Cargo.toml`, `go.mod`, `package.json`...), fichiers de build (`Makefile`, `CMakeLists.txt`...) et extensions source à un score de priorité (30 à 100+). Le langage avec le score le plus élevé gagne. Cas spécial : `package.json` est inspecté pour distinguer React/Vue/Angular/Node par son contenu. Les icônes viennent d'une Nerd Font — vides ou box-drawing sans elle.

5. **Introspection Git** (`is_git_repository`, `get_git_branch`, `get_git_counts`, `build_git_status_string`) — entièrement basée sur `popen()` vers le binaire `git` (pas de `libgit2`). `get_git_branch` essaie dans l'ordre `symbolic-ref` → `describe --tags --exact-match` → `rev-parse --short HEAD`. Chaque appel au binaire relance ces commandes, donc un dépôt Git avec beaucoup d'historique peut ralentir le prompt.

6. **Rendu** — `render_status_line()` construit la ligne d'info (segments SSH/user@host/chemin/langage/git/jobs/load), `render_prompt()` construit la ligne de prompt (code retour + symbole `$`/`#`). `main()` bascule entre les modes (`--setup`, `--status`, `--prompt`, `--simple`, défaut) ; en mode défaut il utilise des séquences curseur (`\033[s`, `\033[<row>;1H`, `\033[u`) pour afficher la ligne de statut tout en bas du terminal puis revenir à la position du prompt.

## Points d'attention spécifiques au repo

- **Fichiers générés trackés en Git** : le binaire compilé `prompt-decorator` et le fichier `2` (capture de sortie debug) sont actuellement suivis par Git — probablement à ignorer plutôt qu'à committer. `bash-prompt-decorator.c.bak` est une sauvegarde non trackée qui traîne dans le répertoire de travail.
- **Langue** : README, spec (`docs/spec.md`) et commentaires du code source sont en français — garder cette cohérence en éditant.
- **`_GNU_SOURCE`** est requis (déjà défini en tête de fichier) pour `getloadavg()`.
- **Sécurité `exec_cmd`** : les commandes Git sont construites via `snprintf` puis passées à `popen()`. Un chemin (`cwd`) contenant un guillemet simple casserait la commande shell construite — attention si on touche à cette fonction ou aux appels qui interpolent un chemin utilisateur.
- **`install.sh`** modifie directement `~/.bashrc` par append (`>>`) — ne pas le relancer sans réfléchir, il n'est pas idempotent.
