PROMPT_COMMAND='__update_prompt'

__update_prompt() {
    local last_exit=$?
    local jobs=$(jobs -p | wc -l)
    export JOBS="$jobs"
    export LAST_EXIT="$last_exit"
    PS1=$(/usr/local/bin/prompt-decorator 2>/dev/null || echo "\u@\h:\w\$ ")
}

