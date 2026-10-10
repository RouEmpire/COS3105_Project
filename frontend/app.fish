#!/usr/bin/env fish

# Ensure script runs from project root directory
set SCRIPT_DIR (status dirname)
set ROOT_DIR (cd $SCRIPT_DIR/..; and pwd)
cd $ROOT_DIR

# Helper function to send TCP command to server using netcat
function send_tcp_command
    set -l cmd $argv[1]
    set -l res (echo $cmd | ncat -N -w 2  127.0.0.1 3030 2>/dev/null)
    echo $res
end

# Feature [1]: Get Board
function handle_get_board
    echo ""
    set_color cyan --bold
    echo "=== [1] GET BOARD ==="
    set_color normal
    read -P "Enter Board ID > " board_id
    set board_id (string trim -- $board_id)

    if test -z "$board_id"
        set_color red
        echo "Error: Board ID cannot be empty."
        set_color normal
        return
    end

    set -l response (send_tcp_command "GET_BOARD,$board_id")

    if test -z "$response"
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*GET_BOARD,0,*" -- $response; or string match -q "*GET_BOARDS,0,*" -- $response
        set -l title (string replace -r '^GET_BOARD[S]?,0,' '' -- $response)
        set_color green --bold
        echo "SUCCESS: Board Found [ID: $board_id]"
        set_color normal
        echo "Board Name: $title"
    else if string match -q "*201*" -- $response
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
        set_color normal
    else if string match -q "*200*" -- $response
        set_color yellow
        echo "INVALID ARGUMENTS: Board ID is invalid (Status 200)."
        set_color normal
    else
        set_color yellow
        echo "Server Response: $response"
        set_color normal
    end
end

# Feature [2]: Get Task
function handle_get_task
    echo ""
    set_color cyan --bold
    echo "=== [2] GET TASK ==="
    set_color normal
    read -P "Enter Board ID > " board_id
    set board_id (string trim -- $board_id)

    if test -z "$board_id"
        set_color red
        echo "Error: Board ID cannot be empty."
        set_color normal
        return
    end

    read -P "Enter Task ID (optional, press Enter for all tasks) > " task_id
    set task_id (string trim -- $task_id)

    set -l cmd ""
    if test -z "$task_id"
        set cmd "GET_TASKS,$board_id"
    else
        set cmd "GET_TASK,$board_id,$task_id"
    end

    set -l response (send_tcp_command $cmd)

    if test -z "$response"
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*0,*" -- $response
        set_color green --bold
        echo "SUCCESS: Task Info Received"
        set_color normal
        echo "Response: $response"
    else if string match -q "*201*" -- $response
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
        set_color normal
    else if string match -q "*203*" -- $response
        set_color yellow
        echo "NOT FOUND: Task ID '$task_id' does not exist (Status 203)."
        set_color normal
    else
        set_color yellow
        echo "Server Response: $response"
        set_color normal
    end
end

# Feature [3]: Add Board
function handle_add_board
    echo ""
    set_color cyan --bold
    echo "=== [3] ADD BOARD ==="
    set_color normal
    read -P "Enter Board Title > " board_title
    set board_title (string trim -- $board_title)

    if test -z "$board_title"
        set_color red
        echo "Error: Board Title cannot be empty."
        set_color normal
        return
    end

    set -l response (send_tcp_command "ADD_BOARD,$board_title")

    if test -z "$response"
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*ADD_BOARD,0*" -- $response
        set_color green --bold
        echo "SUCCESS: Board '$board_title' created successfully!"
        set_color normal
    else if string match -q "*200*" -- $response
        set_color yellow
        echo "INVALID ARGUMENTS: Board Title is invalid (Status 200)."
        set_color normal
    else
        set_color yellow
        echo "Server Response: $response"
        set_color normal
    end
end

# Feature [T]: Run Test
function handle_run_tests
    echo ""
    set_color cyan --bold
    echo "=== [T] RUNNING TEST SUITES ==="
    set_color normal

    echo ""
    set_color yellow
    echo "[1/3] Running test_add_board..."
    set_color normal
    ./run_test.sh test_add_board

    echo ""
    set_color yellow
    echo "[2/3] Running test_get_board..."
    set_color normal
    ./run_test.sh test_get_board

    echo ""
    set_color yellow
    echo "[3/3] Running test_models..."
    set_color normal
    ./run_test.sh test_models

    echo ""
    set_color green --bold
    echo "✅ All tests execution completed!"
    set_color normal
end

# Main Interactive CLI Prompt Loop
while true
    echo ""
    set_color cyan --bold
    echo "--- Zeen's Kanban Board ---"
    set_color normal
    echo "> [1] Get board"
    echo "> [2] Get Task"
    echo "> [3] Add board"
    echo "> [T] Run Test"
    echo "> [X] Exit"
    set_color magenta
    echo "--- # --- # ---"
    set_color normal
    read -P "[COMMAND] > " choice

    set choice (string trim -- $choice)

    switch $choice
        case 1
            handle_get_board
        case 2
            handle_get_task
        case 3
            handle_add_board
        case T t
            handle_run_tests
        case X x
            echo ""
            set_color green --bold
            echo "Exiting Zeen's Kanban Board. Goodbye!"
            set_color normal
            break
        case '*'
            set_color red
            echo "Invalid choice '$choice'. Please enter 1, 2, 3, T, or X."
            set_color normal
    end
end
