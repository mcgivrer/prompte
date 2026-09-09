/*
 * bash-prompt-decorator.c
 * Classe C (style orienté objet en C) pour décorer un terminal bash
 * Compatible GCC, POSIX
 * 
 * Compilation : gcc -o prompt-decorator bash-prompt-decorator.c
 * Utilisation : eval $(./prompt-decorator)
 * Ou dans .bashrc : PS1='$(/chemin/vers/prompt-decorator)'
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>

/* ============================================================
 *   CONSTANTES DE COULEURS ANSI
 * ============================================================ */
#define COLOR_RESET     "\\[\\033[0m\\]"
#define COLOR_BOLD      "\\[\\033[1m\\]"
#define COLOR_DIM       "\\[\\033[2m\\]"
#define COLOR_RED       "\\[\\033[31m\\]"
#define COLOR_GREEN     "\\[\\033[32m\\]"
#define COLOR_YELLOW    "\\[\\033[33m\\]"
#define COLOR_BLUE      "\\[\\033[34m\\]"
#define COLOR_MAGENTA   "\\[\\033[35m\\]"
#define COLOR_CYAN      "\\[\\033[36m\\]"
#define COLOR_BLACK     "\\[\\033[30m\\]"
#define COLOR_WHITE     "\\[\\033[37m\\]"
#define COLOR_ORANGE    "\\[\\033[38;5;208m\\]"
#define COLOR_LIME      "\\[\\033[38;5;118m\\]"
#define COLOR_PINK      "\\[\\033[38;5;205m\\]"
#define BG_RED          "\\[\\033[41m\\]"
#define BG_GREEN        "\\[\\033[42m\\]"
#define BG_YELLOW       "\\[\\033[43m\\]"
#define BG_BLUE         "\\[\\033[44m\\]"
#define BG_MAGENTA      "\\[\\033[45m\\]"
#define BG_CYAN         "\\[\\033[46m\\]"

/* ============================================================
 *   CONSTANTES ANSI BRUTES (pour affichage direct hors PS1)
 * ============================================================ */
#define RAW_RESET       "\033[0m"
#define RAW_BOLD        "\033[1m"
#define RAW_DIM         "\033[2m"
#define RAW_RED         "\033[31m"
#define RAW_GREEN       "\033[32m"
#define RAW_YELLOW      "\033[33m"
#define RAW_BLUE        "\033[34m"
#define RAW_MAGENTA     "\033[35m"
#define RAW_CYAN        "\033[36m"
#define RAW_BLACK       "\033[30m"
#define RAW_WHITE       "\033[37m"
#define RAW_ORANGE      "\033[38;5;208m"
#define RAW_LIME        "\033[38;5;118m"
#define RAW_PINK        "\033[38;5;205m"
#define RAW_BG_RED      "\033[41m"
#define RAW_BG_GREEN    "\033[42m"
#define RAW_BG_YELLOW   "\033[43m"
#define RAW_BG_BLUE     "\033[44m"
#define RAW_BG_MAGENTA  "\033[45m"
#define RAW_BG_CYAN     "\033[46m"



/* ============================================================
 *   TYPES & STRUCTURES
 * ============================================================ */

typedef enum {
    LANG_UNKNOWN = 0,
    LANG_C,
    LANG_CPP,
    LANG_PYTHON,
    LANG_RUST,
    LANG_GO,
    LANG_JAVA,
    LANG_JS,
    LANG_TS,
    LANG_RUBY,
    LANG_PHP,
    LANG_SWIFT,
    LANG_KOTLIN,
    LANG_SHELL,
    LANG_LUA,
    LANG_PERL,
    LANG_HASKELL,
    LANG_ELIXIR,
    LANG_DART,
    LANG_FLUTTER,
    LANG_NODEJS,
    LANG_REACT,
    LANG_ANGULAR,
    LANG_VUE,
    LANG_DOCKER,
    LANG_MAKE,
    LANG_CMAKE,
    LANG_MESON,
    LANG_NINJA,
    LANG_COUNT
} ProjectLang;

typedef struct {
    const char *name;
    const char *icon;
    const char *color;
    const char *bg_color;
} LangInfo;

typedef struct {
    char username[64];
    char hostname[64];
    char cwd[4096];
    char home[4096];
    char short_cwd[256];
    char git_branch[128];
    char git_status[256];
    char git_status_raw[256];
    int  git_ahead;
    int  git_behind;
    int  git_modified;
    int  git_staged;
    int  git_untracked;
    int  git_stashed;
    int  is_git_repo;
    ProjectLang lang;
    int  error_code;
    int  is_root;
    int  ssh_session;
    int  jobs_count;
    double load_avg[3];
} PromptContext;

typedef struct {
    char buffer[8192];
    size_t pos;
} PromptBuilder;

/* ============================================================
 *   TABLE DES LANGAGES
 * ============================================================ */

static const LangInfo LANG_TABLE[LANG_COUNT] = {
    [LANG_UNKNOWN]  = {"?",       "◆", COLOR_WHITE,   ""},
    [LANG_C]        = {"C",       "", COLOR_BLUE,    ""},
    [LANG_CPP]      = {"C++",     "", COLOR_BLUE,    ""},
    [LANG_PYTHON]   = {"Python",  "", COLOR_YELLOW,  ""},
    [LANG_RUST]     = {"Rust",    "", COLOR_ORANGE,  ""},
    [LANG_GO]       = {"Go",      "", COLOR_CYAN,    ""},
    [LANG_JAVA]     = {"Java",    "", COLOR_RED,     ""},
    [LANG_JS]       = {"JS",      "", COLOR_YELLOW,  ""},
    [LANG_TS]       = {"TS",      "", COLOR_BLUE,    ""},
    [LANG_RUBY]     = {"Ruby",    "", COLOR_RED,     ""},
    [LANG_PHP]      = {"PHP",     "", COLOR_MAGENTA, ""},
    [LANG_SWIFT]    = {"Swift",   "", COLOR_ORANGE,  ""},
    [LANG_KOTLIN]   = {"Kotlin",  "", COLOR_MAGENTA, ""},
    [LANG_SHELL]    = {"Shell",   "", COLOR_GREEN,   ""},
    [LANG_LUA]      = {"Lua",     "", COLOR_BLUE,    ""},
    [LANG_PERL]     = {"Perl",    "", COLOR_MAGENTA, ""},
    [LANG_HASKELL]  = {"Haskell", "", COLOR_MAGENTA, ""},
    [LANG_ELIXIR]   = {"Elixir",  "", COLOR_MAGENTA, ""},
    [LANG_DART]     = {"Dart",    "", COLOR_CYAN,    ""},
    [LANG_FLUTTER]  = {"Flutter", "", COLOR_CYAN,    ""},
    [LANG_NODEJS]   = {"Node",    "", COLOR_GREEN,   ""},
    [LANG_REACT]    = {"React",   "", COLOR_CYAN,    ""},
    [LANG_ANGULAR]  = {"Angular", "", COLOR_RED,     ""},
    [LANG_VUE]      = {"Vue",     "", COLOR_GREEN,   ""},
    [LANG_DOCKER]   = {"Docker",  "", COLOR_BLUE,    ""},
    [LANG_MAKE]     = {"Make",    "", COLOR_WHITE,   ""},
    [LANG_CMAKE]    = {"CMake",   "", COLOR_BLUE,    ""},
    [LANG_MESON]    = {"Meson",   "󰔷", COLOR_GREEN,   ""},
    [LANG_NINJA]    = {"Ninja",   "󰷖", COLOR_YELLOW,  ""},
};

/* ============================================================
 *   PROMPT BUILDER
 * ============================================================ */

static void pb_init(PromptBuilder *pb) {
    pb->pos = 0;
    pb->buffer[0] = '\0';
}

static void pb_append(PromptBuilder *pb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(pb->buffer + pb->pos, sizeof(pb->buffer) - pb->pos, fmt, args);
    va_end(args);
    if (n > 0) pb->pos += (size_t)n;
}

static void pb_segment(PromptBuilder *pb, const char *bg, const char *fg, 
                       const char *icon, const char *text) {
    pb_append(pb, "%s%s %s%s %s", bg, fg, icon, text, COLOR_RESET);
}

static void pb_raw(PromptBuilder *pb, const char *str) {
    pb_append(pb, "%s", str);
}

/* ============================================================
 *   UTILITAIRES
 * ============================================================ */

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int dir_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void exec_cmd(const char *cmd, char *out, size_t out_size) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        out[0] = '\0';
        return;
    }
    if (fgets(out, (int)out_size, fp)) {
        size_t len = strlen(out);
        if (len > 0 && out[len-1] == '\n') out[len-1] = '\0';
    } else {
        out[0] = '\0';
    }
    pclose(fp);
}

static void get_short_path(const char *cwd, const char *home, char *out, size_t out_size) {
    size_t home_len = strlen(home);
    if (strncmp(cwd, home, home_len) == 0) {
        snprintf(out, out_size, "~%s", cwd + home_len);
    } else {
        strncpy(out, cwd, out_size - 1);
        out[out_size - 1] = '\0';
    }
}

static int get_terminal_width(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    const char *cols = getenv("COLUMNS");
    if (cols) {
        int w = atoi(cols);
        if (w > 0) return w;
    }
    return 80;
}

static size_t visible_strlen(const char *s) {
    size_t len = 0;
    while (*s) {
        if (*s == '\\' && s[1] == '[') {
            s += 2;
            while (*s && !(*s == '\\' && s[1] == ']')) s++;
            if (*s) s += 2;
        } else if (*s == '\\' && s[1] == '0' && s[2] == '3' && s[3] == '3') {
            s += 4;
            while (*s && *s != 'm') s++;
            if (*s) s++;
        } else if (*s == '\033') {
            while (*s && *s != 'm') s++;
            if (*s) s++;
        } else {
            len++;
            s++;
        }
    }
    return len;
}

/* ============================================================
 *   DETECTION DU PROJET / LANGAGE
 * ============================================================ */

static ProjectLang detect_language(const char *cwd) {
    struct {
        const char *file;
        ProjectLang lang;
        int priority;
    } markers[] = {
        /* Haute priorité : fichiers de config spécifiques */
        {"Cargo.toml",        LANG_RUST,     100},
        {"go.mod",            LANG_GO,       100},
        {"package.json",      LANG_NODEJS,   90},
        {"composer.json",     LANG_PHP,      90},
        {"Gemfile",           LANG_RUBY,     90},
        {"pom.xml",           LANG_JAVA,     90},
        {"build.gradle",      LANG_JAVA,     90},
        {"setup.py",          LANG_PYTHON,   90},
        {"pyproject.toml",    LANG_PYTHON,   95},
        {"requirements.txt",  LANG_PYTHON,   80},
        {"Pipfile",           LANG_PYTHON,   85},
        {"CMakeLists.txt",    LANG_CMAKE,    90},
        {"meson.build",       LANG_MESON,    90},
        {"build.ninja",       LANG_NINJA,    90},
        {"Makefile",          LANG_MAKE,     70},
        {"Dockerfile",        LANG_DOCKER,   80},
        {"docker-compose.yml",LANG_DOCKER,   80},
        {"pubspec.yaml",      LANG_FLUTTER,  90},
        {"stack.yaml",        LANG_HASKELL,  90},
        {"mix.exs",           LANG_ELIXIR,   90},

        /* Fichiers source */
        {".rs",               LANG_RUST,     50},
        {".go",               LANG_GO,       50},
        {".py",               LANG_PYTHON,   50},
        {".java",             LANG_JAVA,     50},
        {".kt",               LANG_KOTLIN,   50},
        {".swift",            LANG_SWIFT,    50},
        {".c",                LANG_C,        40},
        {".h",                LANG_C,        30},
        {".cpp",              LANG_CPP,      50},
        {".cc",               LANG_CPP,      50},
        {".hpp",              LANG_CPP,      40},
        {".js",               LANG_JS,       40},
        {".jsx",              LANG_REACT,    60},
        {".ts",               LANG_TS,       50},
        {".tsx",              LANG_REACT,    65},
        {".vue",              LANG_VUE,      60},
        {".rb",               LANG_RUBY,     50},
        {".php",              LANG_PHP,      50},
        {".sh",               LANG_SHELL,    40},
        {".bash",             LANG_SHELL,    40},
        {".lua",              LANG_LUA,      50},
        {".pl",               LANG_PERL,     50},
        {".hs",               LANG_HASKELL,  50},
        {".ex",               LANG_ELIXIR,   50},
        {".exs",              LANG_ELIXIR,   50},
        {".dart",             LANG_DART,     50},

        {NULL, LANG_UNKNOWN, 0}
    };

    int best_score = 0;
    ProjectLang best_lang = LANG_UNKNOWN;

    DIR *dir = opendir(cwd);
    if (!dir) return LANG_UNKNOWN;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        for (int i = 0; markers[i].file != NULL; i++) {
            int is_ext = (markers[i].file[0] == '.');
            int match = 0;

            if (is_ext) {
                size_t namelen = strlen(entry->d_name);
                size_t extlen = strlen(markers[i].file);
                if (namelen > extlen && strcmp(entry->d_name + namelen - extlen, markers[i].file) == 0)
                    match = 1;
            } else {
                if (strcmp(entry->d_name, markers[i].file) == 0)
                    match = 1;
            }

            if (match && markers[i].priority > best_score) {
                best_score = markers[i].priority;
                best_lang = markers[i].lang;
            }
        }
    }
    closedir(dir);

    /* Détection spéciale React/Vue/Angular dans package.json */
    if (best_lang == LANG_NODEJS) {
        char path[4096];
        snprintf(path, sizeof(path), "%s/package.json", cwd);
        FILE *fp = fopen(path, "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "\"react\"")) { best_lang = LANG_REACT; break; }
                if (strstr(line, "\"vue\"")) { best_lang = LANG_VUE; break; }
                if (strstr(line, "\"@angular\"")) { best_lang = LANG_ANGULAR; break; }
            }
            fclose(fp);
        }
    }

    return best_lang;
}

/* ============================================================
 *   DETECTION GIT
 * ============================================================ */

static int is_git_repository(const char *cwd) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "cd '%s' && git rev-parse --is-inside-work-tree 2>/dev/null", cwd);
    char out[8];
    exec_cmd(cmd, out, sizeof(out));
    return strcmp(out, "true") == 0;
}

static void get_git_branch(const char *cwd, char *branch, size_t size) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), 
        "cd '%s' && git symbolic-ref --short HEAD 2>/dev/null || git describe --tags --exact-match 2>/dev/null || git rev-parse --short HEAD 2>/dev/null",
        cwd);
    exec_cmd(cmd, branch, size);
    if (strlen(branch) == 0) {
        strncpy(branch, "unknown", size - 1);
        branch[size - 1] = '\0';
    }
}

static void get_git_counts(const char *cwd, int *ahead, int *behind, int *modified, 
                           int *staged, int *untracked, int *stashed) {
    char cmd[4096];
    char out[256];

    *ahead = *behind = *modified = *staged = *untracked = *stashed = 0;

    /* Ahead/Behind */
    snprintf(cmd, sizeof(cmd), 
        "cd '%s' && git rev-list --left-right --count HEAD...@{upstream} 2>/dev/null", cwd);
    exec_cmd(cmd, out, sizeof(out));
    if (strlen(out) > 0) {
        sscanf(out, "%d\t%d", ahead, behind);
    }

    /* Modified */
    snprintf(cmd, sizeof(cmd), "cd '%s' && git diff --name-only 2>/dev/null | wc -l", cwd);
    exec_cmd(cmd, out, sizeof(out));
    *modified = atoi(out);

    /* Staged */
    snprintf(cmd, sizeof(cmd), "cd '%s' && git diff --cached --name-only 2>/dev/null | wc -l", cwd);
    exec_cmd(cmd, out, sizeof(out));
    *staged = atoi(out);

    /* Untracked */
    snprintf(cmd, sizeof(cmd), "cd '%s' && git ls-files --others --exclude-standard 2>/dev/null | wc -l", cwd);
    exec_cmd(cmd, out, sizeof(out));
    *untracked = atoi(out);

    /* Stashed */
    snprintf(cmd, sizeof(cmd), "cd '%s' && git stash list 2>/dev/null | wc -l", cwd);
    exec_cmd(cmd, out, sizeof(out));
    *stashed = atoi(out);
}

static void build_git_status_string(const PromptContext *ctx, char *out, size_t size, int raw) {
    char parts[256] = "";
    const char *c_y = raw ? RAW_YELLOW : COLOR_YELLOW;
    const char *c_g = raw ? RAW_GREEN : COLOR_GREEN;
    const char *c_r = raw ? RAW_RED : COLOR_RED;
    const char *c_m = raw ? RAW_MAGENTA : COLOR_MAGENTA;
    const char *c_c = raw ? RAW_CYAN : COLOR_CYAN;
    const char *c_reset = raw ? RAW_RESET : COLOR_RESET;

    if (ctx->git_modified > 0) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s~%d%s", c_y, ctx->git_modified, c_reset);
        strncat(parts, tmp, sizeof(parts) - strlen(parts) - 1);
    }
    if (ctx->git_staged > 0) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s+%d%s", c_g, ctx->git_staged, c_reset);
        strncat(parts, tmp, sizeof(parts) - strlen(parts) - 1);
    }
    if (ctx->git_untracked > 0) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s?%d%s", c_r, ctx->git_untracked, c_reset);
        strncat(parts, tmp, sizeof(parts) - strlen(parts) - 1);
    }
    if (ctx->git_stashed > 0) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s⚑%d%s", c_m, ctx->git_stashed, c_reset);
        strncat(parts, tmp, sizeof(parts) - strlen(parts) - 1);
    }
    if (ctx->git_ahead > 0) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s↑%d%s", c_c, ctx->git_ahead, c_reset);
        strncat(parts, tmp, sizeof(parts) - strlen(parts) - 1);
    }
    if (ctx->git_behind > 0) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), " %s↓%d%s", c_c, ctx->git_behind, c_reset);
        strncat(parts, tmp, sizeof(parts) - strlen(parts) - 1);
    }

    strncpy(out, parts, size - 1);
    out[size - 1] = '\0';
}

/* ============================================================
 *   DETECTION DU CONTEXTE
 * ============================================================ */

static void detect_context(PromptContext *ctx) {
    /* Utilisateur */
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        strncpy(ctx->username, pw->pw_name, sizeof(ctx->username) - 1);
    } else {
        const char *user = getenv("USER");
        strncpy(ctx->username, user ? user : "?", sizeof(ctx->username) - 1);
    }
    ctx->username[sizeof(ctx->username) - 1] = '\0';

    /* Hostname */
    if (gethostname(ctx->hostname, sizeof(ctx->hostname)) != 0) {
        strncpy(ctx->hostname, "?", sizeof(ctx->hostname) - 1);
    }
    ctx->hostname[sizeof(ctx->hostname) - 1] = '\0';

    /* Répertoire courant */
    if (!getcwd(ctx->cwd, sizeof(ctx->cwd))) {
        strncpy(ctx->cwd, "?", sizeof(ctx->cwd) - 1);
    }

    /* Home */
    const char *home = getenv("HOME");
    if (home) {
        strncpy(ctx->home, home, sizeof(ctx->home) - 1);
    } else if (pw) {
        strncpy(ctx->home, pw->pw_dir, sizeof(ctx->home) - 1);
    } else {
        strncpy(ctx->home, "/", sizeof(ctx->home) - 1);
    }
    ctx->home[sizeof(ctx->home) - 1] = '\0';

    get_short_path(ctx->cwd, ctx->home, ctx->short_cwd, sizeof(ctx->short_cwd));

    /* Root ? */
    ctx->is_root = (getuid() == 0);

    /* SSH ? */
    ctx->ssh_session = (getenv("SSH_CLIENT") != NULL || getenv("SSH_TTY") != NULL);

    /* Code erreur dernier commande */
    const char *err_str = getenv("LAST_EXIT");
    ctx->error_code = err_str ? atoi(err_str) : 0;

    /* Load average */
    getloadavg(ctx->load_avg, 3);

    /* Jobs en arrière-plan (bash) */
    const char *jobs = getenv("JOBS");
    ctx->jobs_count = jobs ? atoi(jobs) : 0;

    /* Langage du projet */
    ctx->lang = detect_language(ctx->cwd);

    /* Git */
    ctx->is_git_repo = is_git_repository(ctx->cwd);
    if (ctx->is_git_repo) {
        get_git_branch(ctx->cwd, ctx->git_branch, sizeof(ctx->git_branch));
        get_git_counts(ctx->cwd, &ctx->git_ahead, &ctx->git_behind,
                       &ctx->git_modified, &ctx->git_staged,
                       &ctx->git_untracked, &ctx->git_stashed);
        build_git_status_string(ctx, ctx->git_status, sizeof(ctx->git_status), 0);
        build_git_status_string(ctx, ctx->git_status_raw, sizeof(ctx->git_status_raw), 1);
    } else {
        ctx->git_branch[0] = '\0';
        ctx->git_status[0] = '\0';
        ctx->git_status_raw[0] = '\0';
    }
}

/* ============================================================
 *   RENDU DU PROMPT
 * ============================================================ */

static void render_status_line(const PromptContext *ctx, PromptBuilder *sb) {
    /* [SSH] si session distante */
    if (ctx->ssh_session) {
        pb_append(sb, "%s%s SSH %s", RAW_BG_YELLOW, RAW_BLACK, RAW_RESET);
    }

    /* [user@host] */
    const char *user_bg = ctx->is_root ? RAW_BG_RED : RAW_BG_GREEN;
    pb_append(sb, "%s%s %s@%s%s%s ",
              user_bg, RAW_BOLD RAW_BLACK,
              ctx->username,
              RAW_DIM RAW_BLACK,
              ctx->hostname,
              RAW_RESET);

    /* [chemin] */
    pb_append(sb, "%s%s %s %s",
              RAW_BG_BLUE, RAW_BOLD RAW_WHITE,
              ctx->short_cwd,
              RAW_RESET);

    /* [Langage du projet] */
    if (ctx->lang != LANG_UNKNOWN) {
        const LangInfo *li = &LANG_TABLE[ctx->lang];
        pb_append(sb, "%s%s %s %s%s",
                  RAW_BG_MAGENTA, RAW_BOLD RAW_WHITE,
                  li->icon,
                  li->name,
                  RAW_RESET);
    }

    /* [Git] */
    if (ctx->is_git_repo) {
        const char *branch_color = (ctx->git_modified > 0 || ctx->git_untracked > 0) 
                                    ? RAW_YELLOW 
                                    : RAW_GREEN;
        pb_append(sb, "%s%s  %s%s%s%s",
                  RAW_BG_CYAN, RAW_BOLD RAW_BLACK,
                  branch_color,
                  ctx->git_branch,
                  ctx->git_status_raw,
                  RAW_RESET);
    }

    /* [Jobs] */
    if (ctx->jobs_count > 0) {
        pb_append(sb, "%s%s ⚙ %d %s",
                  RAW_BG_YELLOW, RAW_BLACK,
                  ctx->jobs_count,
                  RAW_RESET);
    }

    /* [Load] si élevé */
    if (ctx->load_avg[0] > 4.0) {
        pb_append(sb, "%s%s ⚡ %.1f %s",
                  RAW_BG_RED, RAW_WHITE,
                  ctx->load_avg[0],
                  RAW_RESET);
    }

    /* Date/heure en fin de ligne, video inverse, pleine largeur */
    {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char datetime[64];
        strftime(datetime, sizeof(datetime), "%H:%M  %d/%m/%Y", t);

        int term_width = get_terminal_width();
        size_t line_vis = visible_strlen(sb->buffer);
        size_t date_vis = strlen(datetime);
        int padding = term_width - (int)line_vis - (int)date_vis;
        if (padding < 1) padding = 1;

        pb_append(sb, "[7m%*s%s%s[0m",
                  padding, "",
                  datetime,
                  "");
    }
}

static void render_prompt(const PromptContext *ctx, PromptBuilder *pb) {
    if (ctx->error_code != 0) {
        pb_append(pb, "%s%s ✗ %d %s ",
                  BG_RED, COLOR_WHITE,
                  ctx->error_code,
                  COLOR_RESET);
    }

    if (ctx->is_root) {
        pb_append(pb, "%s%s#%s ", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    } else {
        pb_append(pb, "%s%s$%s ", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    }
}


/* ============================================================
 *   MODE "EXPORT PS1" POUR BASH
 * ============================================================ */

static void print_bash_setup(void) {
    fputs("# Ajoutez ceci dans votre ~/.bashrc :\n", stdout);
    fputs("# -----------------------------------------------------------\n", stdout);
    fputs("PROMPT_COMMAND='__update_prompt'\n", stdout);
    fputs("\n", stdout);
    fputs("__update_prompt() {\n", stdout);
    fputs("    local last_exit=$?\n", stdout);
    fputs("    local jobs=$(jobs -p | wc -l)\n", stdout);
    fputs("    export JOBS=\"$jobs\"\n", stdout);
    fputs("    export LAST_EXIT=\"$last_exit\"\n", stdout);
    fputs("\n", stdout);
    fputs("    # Positionner la ligne d'info en bas du terminal\n", stdout);
    fputs("    local H=$(tput lines)\n", stdout);
    fputs("    printf '\\033[s'\n", stdout);
    fputs("    printf '\\033[%s;1H' \"$H\"\n", stdout);
    fputs("    prompt-decorator --status 2>/dev/null\n", stdout);
    fputs("    printf '\\033[u'\n", stdout);
    fputs("\n", stdout);
    fputs("    # Construire le prompt (juste le symbole)\n", stdout);
    fputs("    PS1='$(prompt-decorator --prompt 2>/dev/null || echo \"\\u@\\h:\\w\\$ \")'\n", stdout);
    fputs("}\n", stdout);
    fputs("\n", stdout);
    fputs("# Re-render la status line lors d'un changement de taille du terminal\n", stdout);
    fputs("__render_status() {\n", stdout);
    fputs("    local H=$(tput lines)\n", stdout);
    fputs("    printf '\\033[s\\033[%s;1H' \"$H\"\n", stdout);
    fputs("    prompt-decorator --status 2>/dev/null\n", stdout);
    fputs("    printf '\\033[u'\n", stdout);
    fputs("}\n", stdout);
    fputs("trap '__render_status' WINCH\n", stdout);
    fputs("\n", stdout);
    fputs("# -----------------------------------------------------------\n", stdout);
}




/* ============================================================
 *   MAIN
 * ============================================================ */

int main(int argc, char *argv[]) {
    /* Mode setup */
    if (argc > 1 && (strcmp(argv[1], "--setup") == 0 || strcmp(argv[1], "-s") == 0)) {
        print_bash_setup();
        return 0;
    }

    /* Mode status : juste la ligne d'info */
    if (argc > 1 && (strcmp(argv[1], "--status") == 0)) {
        PromptContext ctx = {0};
        PromptBuilder sb;
        detect_context(&ctx);
        pb_init(&sb);
        render_status_line(&ctx, &sb);
        printf("%s", sb.buffer);
        return 0;
    }

    /* Mode prompt : juste le symbole de prompt */
    if (argc > 1 && (strcmp(argv[1], "--prompt") == 0)) {
        PromptContext ctx = {0};
        PromptBuilder pb;
        detect_context(&ctx);
        pb_init(&pb);
        render_prompt(&ctx, &pb);
        printf("%s", pb.buffer);
        return 0;
    }

    /* Mode simple : juste afficher le prompt */
    if (argc > 1 && (strcmp(argv[1], "--simple") == 0 || strcmp(argv[1], "-S") == 0)) {
        PromptContext ctx = {0};
        PromptBuilder pb;
        detect_context(&ctx);
        pb_init(&pb);

        /* Prompt minimal : user@host:path (git) $ */
        printf("%s%s%s@%s%s%s:%s%s%s",
               COLOR_GREEN, ctx.username, COLOR_RESET,
               COLOR_GREEN, ctx.hostname, COLOR_RESET,
               COLOR_BLUE, ctx.short_cwd, COLOR_RESET);

        if (ctx.is_git_repo) {
            printf(" (%s%s%s%s)",
                   COLOR_MAGENTA, ctx.git_branch, ctx.git_status, COLOR_RESET);
        }

        printf(" %s$%s ", ctx.is_root ? COLOR_RED : COLOR_GREEN, COLOR_RESET);
        return 0;
    }

    /* Mode par defaut : status en bas + prompt au-dessus */
    PromptContext ctx = {0};
    PromptBuilder sb, pb;

    detect_context(&ctx);
    pb_init(&sb);
    pb_init(&pb);
    render_status_line(&ctx, &sb);
    render_prompt(&ctx, &pb);

    /* Obtenir la hauteur du terminal */
    struct winsize ws;
    int term_height = 24;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0)
        term_height = ws.ws_row;

    /* Placer le curseur en bas, afficher la ligne d'info, remonter */
    printf("[s");                     /* sauver position */
    printf("[%d;1H", term_height);   /* curseur en bas */
    printf("%s", sb.buffer);             /* ligne d'info */
    printf("[u");                     /* restaurer position */

    /* Afficher le prompt */
    printf("%s", pb.buffer);

    return 0;
}

