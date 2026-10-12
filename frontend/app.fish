#!/usr/bin/env fish

# Ensure script runs from project root directory
set SCRIPT_DIR (status dirname)
set ROOT_DIR (cd $SCRIPT_DIR/..; and pwd)
cd $ROOT_DIR

# Helper function to send TCP command to server using netcat
function send_tcp_command
    set -l cmd $argv[1]
    echo $cmd | ncat -w 2 127.0.0.1 3030 2>/dev/null
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

# Feature [ALL]: Get All Data (Thread Pool: 8 Workers)
function handle_get_all_data
    echo ""
    set_color cyan --bold
    echo "=== [ALL] GET ALL DATA (THREAD POOL: 8 WORKERS) ==="
    set_color normal

    # Option 1: Limit the board to display
    read -P "Limit the board to display (e.g. 5, or press Enter for ALL) > " board_limit
    set board_limit (string trim -- $board_limit)
    if test -z "$board_limit"; or not string match -qr '^[0-9]+$' -- $board_limit
        set board_limit 0
    end

    # Option 2: Limit the task of each board to display
    read -P "Limit the task of each board to display (e.g. 3, or press Enter for ALL) > " task_limit
    set task_limit (string trim -- $task_limit)
    if test -z "$task_limit"; or not string match -qr '^[0-9]+$' -- $task_limit
        set task_limit 0
    end

    echo ""
    set_color yellow
    echo "Querying server via 8 parallel worker threads (Board Limit: $board_limit, Task Limit: $task_limit)..."
    set_color normal

    set -l response (send_tcp_command "GET_ALL,$board_limit,$task_limit")

    if test (count $response) -eq 0
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
        return
    end

    set -l total_boards 0
    set -l total_tasks 0

    echo ""
    set_color cyan --bold
    echo "==================== KANBAN BOARDS & TASKS ===================="
    set_color normal

    for line in $response
        set -l line (string trim -- $line)
        if test -z "$line"; or test "$line" = "END_ALL"
            continue
        end

        if string match -qr '^BOARD,' -- $line
            set -l parts (string split "," -- $line)
            set -l b_id $parts[2]
            set -l b_title $parts[3]
            set -l b_tasks $parts[4]
            set total_boards (math $total_boards + 1)
            echo ""
            set_color blue --bold
            echo "📂 Board [ID $b_id]: $b_title (Tasks: $b_tasks)"
            set_color normal
        else if string match -qr '^TASK,' -- $line
            set -l parts (string split "," -- $line)
            set -l t_id $parts[2]
            set -l t_title $parts[3]
            set -l t_finish $parts[4]
            set total_tasks (math $total_tasks + 1)
            if test "$t_finish" = "1"
                set_color green
                echo "   ├── [✓] Task #$t_id: $t_title"
                set_color normal
            else
                set_color yellow
                echo "   ├── [ ] Task #$t_id: $t_title"
                set_color normal
            end
        end
    end

    echo ""
    set_color cyan --bold
    echo "==============================================================="
    set_color green --bold
    echo "✅ Summary: Retrieved $total_boards Board(s) and $total_tasks Task(s) via 8 Worker Threads."
    set_color normal
end

# Feature [4]: Add Task
function handle_add_task
    echo ""
    set_color cyan --bold
    echo "=== [4] ADD TASK ==="
    set_color normal

    # Option 1: Board ID (that task will belong to)
    read -P "Enter Board ID > " board_id
    set board_id (string trim -- $board_id)

    if test -z "$board_id"
        set_color red
        echo "Error: Board ID cannot be empty."
        set_color normal
        return
    end

    # Option 2: Task name
    read -P "Enter Task Name > " task_name
    set task_name (string trim -- $task_name)

    if test -z "$task_name"
        set_color red
        echo "Error: Task Name cannot be empty."
        set_color normal
        return
    end

    set -l response (send_tcp_command "ADD_TASK,$board_id,$task_name")

    if test (count $response) -eq 0
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*ADD_TASK,0*" -- $response[1]
        set -l parts (string split "," -- $response[1])
        set -l task_id $parts[3]
        set_color green --bold
        if test -n "$task_id"
            echo "SUCCESS: Task '$task_name' created successfully on Board ID $board_id (Task ID: $task_id)!"
        else
            echo "SUCCESS: Task '$task_name' created successfully on Board ID $board_id!"
        end
        set_color normal
    else if string match -q "*201*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
        set_color normal
    else if string match -q "*Task already exists*" -- $response[1]
        set_color yellow
        echo "DUPLICATE: A task with name '$task_name' already exists on Board ID $board_id."
        set_color normal
    else if string match -q "*200*" -- $response[1]
        set_color yellow
        echo "INVALID ARGUMENTS: Invalid arguments provided (Status 200)."
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
    echo "[1/5] Running test_add_board..."
    set_color normal
    ./run_test.sh test_add_board

    echo ""
    set_color yellow
    echo "[2/5] Running test_add_task..."
    set_color normal
    ./run_test.sh test_add_task

    echo ""
    set_color yellow
    echo "[3/5] Running test_get_board..."
    set_color normal
    ./run_test.sh test_get_board

    echo ""
    set_color yellow
    echo "[4/5] Running test_models..."
    set_color normal
    ./run_test.sh test_models

    echo ""
    set_color yellow
    echo "[5/5] Running test_get_all_data..."
    set_color normal
    ./run_test.sh test_get_all_data

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
    echo "> [4] Add Task"
    echo "> [ALL] Get all Data"
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
        case 4
            handle_add_task
        case ALL all
            handle_get_all_data
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
            echo "Invalid choice '$choice'. Please enter 1, 2, 3, 4, ALL, T, or X."
            set_color normal
    end
end


