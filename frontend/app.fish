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

# Feature [5]: Update Board
function handle_update_board
    echo ""
    set_color cyan --bold
    echo "=== [5] UPDATE BOARD ==="
    set_color normal

    # Identifier: Board ID
    read -P "Enter Board ID to update > " board_id
    set board_id (string trim -- $board_id)

    if test -z "$board_id"
        set_color red
        echo "Error: Board ID cannot be empty."
        set_color normal
        return
    end

    # New Board Name
    read -P "Enter New Board Name > " new_board_name
    set new_board_name (string trim -- $new_board_name)

    if test -z "$new_board_name"
        set_color red
        echo "Error: Board Name cannot be empty."
        set_color normal
        return
    end

    set -l response (send_tcp_command "UPDATE_BOARD,$board_id,$new_board_name")

    if test (count $response) -eq 0
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*UPDATE_BOARD,0*" -- $response[1]
        set_color green --bold
        echo "SUCCESS: Board [ID: $board_id] updated successfully to '$new_board_name'!"
        set_color normal
    else if string match -q "*201*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
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

# Feature [6]: Update Task
function handle_update_task
    echo ""
    set_color cyan --bold
    echo "=== [6] UPDATE TASK ==="
    set_color normal

    # Identifier 1: Board ID
    read -P "Enter Board ID > " board_id
    set board_id (string trim -- $board_id)

    if test -z "$board_id"
        set_color red
        echo "Error: Board ID cannot be empty."
        set_color normal
        return
    end

    # Identifier 2: Task ID
    read -P "Enter Task ID to update > " task_id
    set task_id (string trim -- $task_id)

    if test -z "$task_id"
        set_color red
        echo "Error: Task ID cannot be empty."
        set_color normal
        return
    end

    # New Task Name
    read -P "Enter New Task Name > " new_task_name
    set new_task_name (string trim -- $new_task_name)

    if test -z "$new_task_name"
        set_color red
        echo "Error: Task Name cannot be empty."
        set_color normal
        return
    end

    set -l response (send_tcp_command "UPDATE_TASK,$board_id,$task_id,$new_task_name")

    if test (count $response) -eq 0
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*UPDATE_TASK,0*" -- $response[1]
        set_color green --bold
        echo "SUCCESS: Task [ID: $task_id] on Board [ID: $board_id] updated successfully to '$new_task_name'!"
        set_color normal
    else if string match -q "*201*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
        set_color normal
    else if string match -q "*203*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Task ID '$task_id' does not exist on Board ID '$board_id' (Status 203)."
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

# Feature [8]: Soft Delete Board
function handle_delete_board
    echo ""
    set_color cyan --bold
    echo "=== [8] SOFT DELETE BOARD ==="
    set_color normal

    # DELETE_BOARD accepts exactly one option: a positive numeric board ID.
    read -P "Enter Board ID to soft delete > " board_id
    set board_id (string trim -- $board_id)

    if not string match -qr '^[1-9][0-9]*$' -- $board_id
        set_color red
        echo "Error: Board ID must be a positive number."
        set_color normal
        return
    end

    # The server replaces this board's board.txt line with "---".
    set -l response (send_tcp_command "DELETE_BOARD,$board_id")

    if test (count $response) -eq 0
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*DELETE_BOARD,0*" -- $response[1]
        set_color green --bold
        echo "SUCCESS: Board [ID: $board_id] was soft deleted."
        set_color normal
        echo "Its record in board.txt is now marked as '---'."
    else if string match -q "*201*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
        set_color normal
    else if string match -q "*200*" -- $response[1]
        set_color yellow
        echo "INVALID ARGUMENTS: Board ID is invalid (Status 200)."
        set_color normal
    else
        set_color yellow
        echo "Server Response: $response"
        set_color normal
    end
end

# Feature [9]: Soft Delete Task
function handle_delete_task
    echo ""
    set_color cyan --bold
    echo "=== [9] SOFT DELETE TASK ==="
    set_color normal

    # DELETE_TASK requires two positive numeric identifiers.
    read -P "Enter Board ID > " board_id
    set board_id (string trim -- $board_id)
    read -P "Enter Task ID to soft delete > " task_id
    set task_id (string trim -- $task_id)

    if not string match -qr '^[1-9][0-9]*$' -- $board_id
        set_color red
        echo "Error: Board ID must be a positive number."
        set_color normal
        return
    end

    if not string match -qr '^[1-9][0-9]*$' -- $task_id
        set_color red
        echo "Error: Task ID must be a positive number."
        set_color normal
        return
    end

    set -l response (send_tcp_command "DELETE_TASK,$board_id,$task_id")

    if test (count $response) -eq 0
        set_color red
        echo "Error: Unable to connect to server on 127.0.0.1:3030."
        echo "Please make sure ./run_server.sh is running in another terminal."
        set_color normal
    else if string match -q "*DELETE_TASK,0*" -- $response[1]
        set_color green --bold
        echo "SUCCESS: Task [ID: $task_id] on Board [ID: $board_id] was soft deleted."
        set_color normal
    else if string match -q "*201*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Board ID '$board_id' does not exist (Status 201)."
        set_color normal
    else if string match -q "*203*" -- $response[1]
        set_color yellow
        echo "NOT FOUND: Task ID '$task_id' does not exist on Board ID '$board_id' (Status 203)."
        set_color normal
    else if string match -q "*200*" -- $response[1]
        set_color yellow
        echo "INVALID ARGUMENTS: Board ID and Task ID must be valid (Status 200)."
        set_color normal
    else if string match -q "UNKNOWN,3*" -- $response[1]
        set_color red --bold
        echo "ERROR: The server does not recognize DELETE_TASK."
        echo "Stop the running server and restart it with ./run_server.sh to rebuild the latest command handlers."
        set_color normal
    else
        set_color yellow
        echo "Server Response: $response"
        set_color normal
    end
end

# Feature [7]: Generate Fake Data
function handle_generate_fake_data
    echo ""
    set_color cyan --bold
    echo "=== [7] GENERATE FAKE DATA ==="
    set_color normal
    echo "This calls generate_fake_data.py with the options you provide."
    echo "Press Enter on any option to use its default value."
    echo ""

    # --- Option 1: Number of boards ---
    read -P "Number of boards to generate [default: 1000] > " num_boards
    set num_boards (string trim -- $num_boards)
    # Validate: must be a positive integer, or empty (use default)
    if test -n "$num_boards"
        if not string match -qr '^[1-9][0-9]*$' -- $num_boards
            set_color red
            echo "Error: Number of boards must be a positive integer. Got: '$num_boards'"
            set_color normal
            return
        end
    else
        set num_boards 1000
    end

    # --- Option 2: Tasks per board ---
    read -P "Number of tasks per board [default: 300] > " tasks_per_board
    set tasks_per_board (string trim -- $tasks_per_board)
    if test -n "$tasks_per_board"
        if not string match -qr '^[1-9][0-9]*$' -- $tasks_per_board
            set_color red
            echo "Error: Tasks per board must be a positive integer. Got: '$tasks_per_board'"
            set_color normal
            return
        end
    else
        set tasks_per_board 300
    end

    # --- Option 3: Board output file ---
    read -P "Board output file [default: board.txt] > " board_file
    set board_file (string trim -- $board_file)
    if test -z "$board_file"
        set board_file "board.txt"
    end

    # --- Option 4: Task output file ---
    read -P "Task output file [default: task.txt] > " task_file
    set task_file (string trim -- $task_file)
    if test -z "$task_file"
        set task_file "task.txt"
    end

    # --- Confirmation summary before running ---
    echo ""
    set_color yellow
    echo "About to generate:"
    echo "  Boards       : $num_boards"
    echo "  Tasks/Board  : $tasks_per_board"
    set_color yellow
    echo "  Total Tasks  : "(math $num_boards \* $tasks_per_board)
    echo "  Board file   : $board_file"
    echo "  Task file    : $task_file"
    set_color normal

    read -P "Proceed? [Y/n] > " confirm
    set confirm (string trim -- $confirm | string lower)
    if test "$confirm" = "n"; or test "$confirm" = "no"
        set_color yellow
        echo "Cancelled."
        set_color normal
        return
    end

    # --- Run the Python generator with chosen options ---
    echo ""
    set_color cyan
    echo "Running generate_fake_data.py..."
    set_color normal

    python3 ./generate_fake_data.py \
        --boards $num_boards \
        --tasks $tasks_per_board \
        --board-file $board_file \
        --task-file $task_file
    set -l py_status $status

    if test $py_status -eq 0
        echo ""
        set_color green --bold
        echo "✅ Fake data generation completed successfully!"
        set_color normal
    else
        echo ""
        set_color red
        echo "❌ generate_fake_data.py exited with an error (exit code $py_status)."
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
    echo "[1/7] Running test_add_board..."
    set_color normal
    ./run_test.sh test_add_board

    echo ""
    set_color yellow
    echo "[2/7] Running test_add_task..."
    set_color normal
    ./run_test.sh test_add_task

    echo ""
    set_color yellow
    echo "[3/7] Running test_update_board..."
    set_color normal
    ./run_test.sh test_update_board

    echo ""
    set_color yellow
    echo "[4/7] Running test_update_task..."
    set_color normal
    ./run_test.sh test_update_task

    echo ""
    set_color yellow
    echo "[5/7] Running test_get_board..."
    set_color normal
    ./run_test.sh test_get_board

    echo ""
    set_color yellow
    echo "[6/7] Running test_models..."
    set_color normal
    ./run_test.sh test_models

    echo ""
    set_color yellow
    echo "[7/7] Running test_get_all_data..."
    set_color normal
    ./run_test.sh test_get_all_data

    echo ""
    set_color green --bold
    echo "✅ All tests execution completed!"
    set_color normal
end

# Show the response statuses that the selected menu command can return.
# This is a local help action; it does not send a command to the server.
function handle_show_status --argument-names command_choice
    echo ""
    set_color cyan --bold
    echo "=== STATUS CODES FOR COMMAND [$command_choice] ==="
    set_color normal

    switch (string upper -- $command_choice)
        case 1
            echo "0   STATUS_OK - board found"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
        case 2
            echo "0   STATUS_OK - task found or task list returned"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
            echo "202 STATUS_EMPTY_BOARD"
            echo "203 STATUS_TASK_NOT_FOUND"
        case 3
            echo "0   STATUS_OK - board created"
            echo "1   STATUS_FAIL"
            echo "200 STATUS_INVALID_ARGUMENTS"
        case 4
            echo "0   STATUS_OK - task created"
            echo "1   STATUS_FAIL"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
        case 5
            echo "0   STATUS_OK - board updated"
            echo "1   STATUS_FAIL"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
            echo "500 STATUS_SAVE_BOARD_FAIL"
        case 6
            echo "0   STATUS_OK - task updated"
            echo "1   STATUS_FAIL"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
            echo "203 STATUS_TASK_NOT_FOUND"
            echo "502 STATUS_SAVE_TASK_FAIL"
        case 8
            echo "0   STATUS_OK - board soft deleted"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
            echo "500 STATUS_SAVE_BOARD_FAIL"
        case 9
            echo "0   STATUS_OK - task soft deleted"
            echo "200 STATUS_INVALID_ARGUMENTS"
            echo "201 STATUS_BOARD_NOT_FOUND"
            echo "203 STATUS_TASK_NOT_FOUND"
            echo "502 STATUS_SAVE_TASK_FAIL"
        case ALL all
            echo "0   STATUS_OK - data response streamed"
            echo "1   STATUS_FAIL - request could not be completed"
        case 7 T t X x
            echo "This is a local action and does not return a server protocol status."
        case '*'
            set_color red
            echo "No menu command '$command_choice' has status information."
            set_color normal
            return 1
    end
end

# Send the selected server command and display the status returned by the server.
# This intentionally executes the operation (including mutations such as add/delete).
function handle_return_status --argument-names command_choice
    set -l protocol_command ""

    switch (string upper -- $command_choice)
        case 1
            read -P "Enter Board ID > " board_id
            set board_id (string trim -- $board_id)
            set protocol_command "GET_BOARD,$board_id"
        case 2
            read -P "Enter Board ID > " board_id
            set board_id (string trim -- $board_id)
            read -P "Enter Task ID (press Enter to list tasks) > " task_id
            set task_id (string trim -- $task_id)
            if test -z "$task_id"
                set protocol_command "GET_TASKS,$board_id"
            else
                set protocol_command "GET_TASK,$board_id,$task_id"
            end
        case 3
            read -P "Enter Board Title > " board_title
            set board_title (string trim -- $board_title)
            set protocol_command "ADD_BOARD,$board_title"
        case 4
            read -P "Enter Board ID > " board_id
            set board_id (string trim -- $board_id)
            read -P "Enter Task Title > " task_title
            set task_title (string trim -- $task_title)
            set protocol_command "ADD_TASK,$board_id,$task_title"
        case 5
            read -P "Enter Board ID to update > " board_id
            set board_id (string trim -- $board_id)
            read -P "Enter New Board Title > " board_title
            set board_title (string trim -- $board_title)
            set protocol_command "UPDATE_BOARD,$board_id,$board_title"
        case 6
            read -P "Enter Board ID > " board_id
            set board_id (string trim -- $board_id)
            read -P "Enter Task ID to update > " task_id
            set task_id (string trim -- $task_id)
            read -P "Enter New Task Title > " task_title
            set task_title (string trim -- $task_title)
            set protocol_command "UPDATE_TASK,$board_id,$task_id,$task_title"
        case 8
            read -P "Enter Board ID to soft delete > " board_id
            set board_id (string trim -- $board_id)
            set protocol_command "DELETE_BOARD,$board_id"
        case 9
            read -P "Enter Board ID > " board_id
            set board_id (string trim -- $board_id)
            read -P "Enter Task ID to soft delete > " task_id
            set task_id (string trim -- $task_id)
            set protocol_command "DELETE_TASK,$board_id,$task_id"
        case ALL all
            read -P "Board limit (0 for all) > " board_limit
            set board_limit (string trim -- $board_limit)
            read -P "Task limit per board (0 for all) > " task_limit
            set task_limit (string trim -- $task_limit)
            if test -z "$board_limit"
                set board_limit 0
            end
            if test -z "$task_limit"
                set task_limit 0
            end
            set protocol_command "GET_ALL,$board_limit,$task_limit"
        case 7 T t X x
            set_color yellow
            echo "This menu choice is local and has no server return status."
            set_color normal
            return 1
        case '*'
            set_color red
            echo "No server command is associated with menu choice '$command_choice'."
            set_color normal
            return 1
    end

    if test -z "$protocol_command"
        set_color red
        echo "Could not build a server command for menu choice '$command_choice'."
        set_color normal
        return 1
    end

    echo "Sending: $protocol_command"
    set -l response (send_tcp_command "$protocol_command")
    if test (count $response) -eq 0
        set_color red
        echo "No response received. Check that the server is running on 127.0.0.1:3030 and that ncat is installed."
        set_color normal
        return 1
    end

    # The protocol places the numeric status in the second comma-delimited field.
    set -l response_fields (string split ',' -- $response[1])
    set -l returned_status "Unknown"
    if test (count $response_fields) -ge 2
        set returned_status $response_fields[2]
    end

    set_color cyan --bold
    echo "Server response: $response[1]"
    echo "Returned status: $returned_status"
    set_color normal

    switch $returned_status
        case 0
            echo "Meaning: STATUS_OK"
        case 1
            echo "Meaning: STATUS_FAIL"
        case 3
            echo "Meaning: STATUS_UNKNOWN (the server did not recognize the command)"
        case 200
            echo "Meaning: STATUS_INVALID_ARGUMENTS"
        case 201
            echo "Meaning: STATUS_BOARD_NOT_FOUND"
        case 202
            echo "Meaning: STATUS_EMPTY_BOARD"
        case 203
            echo "Meaning: STATUS_TASK_NOT_FOUND"
        case 204
            echo "Meaning: STATUS_EMPTY_TASK"
        case 500
            echo "Meaning: STATUS_SAVE_BOARD_FAIL"
        case 501
            echo "Meaning: STATUS_ADD_BOARD_FAIL"
        case 502
            echo "Meaning: STATUS_SAVE_TASK_FAIL"
        case 503
            echo "Meaning: STATUS_ADD_TASK_FAIL"
        case '*'
            echo "Meaning: unrecognized status code"
    end
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
    echo "> [5] Update Board"
    echo "> [6] Update Task"
    echo "> [7] Generate Fake Data"
    echo "> [8] Soft Delete Board"
    echo "> [9] Soft Delete Task"
    echo "> [ALL] Get all Data"
    echo "> [TIP] Get useful tip"
    echo "> [T] Run Test"
    echo "> [X] Exit"
    set_color magenta
    echo "--- # --- # ---"
    set_color normal
    read -P "[COMMAND] > " choice

    # Parse the menu choice and its optional status-display flag.
    set -l choice_parts (string split -n ' ' -- (string trim -- $choice))
    set choice $choice_parts[1]

    if test (count $choice_parts) -gt 1
        if test (count $choice_parts) -ne 2
            set_color red
            echo "Invalid arguments. Append only one supported option after a menu choice."
            set_color normal
            continue
        end

        switch $choice_parts[2]
            case --status -s
                handle_show_status $choice
                continue
            case --return-stats -r
                handle_return_status $choice
                continue
            case '*'
                set_color red
                echo "Unknown option '$choice_parts[2]'. Use --status/-s or --return-stats/-r."
                set_color normal
                continue
        end
    end

    switch $choice
        case 1
            handle_get_board
        case 2
            handle_get_task
        case 3
            handle_add_board
        case 4
            handle_add_task
        case 5
            handle_update_board
        case 6
            handle_update_task
        case 7
            handle_generate_fake_data
        case 8
            handle_delete_board
        case 9
            handle_delete_task
        case ALL all All
            handle_get_all_data
        case T t
            handle_run_tests
        case X x
            echo ""
            set_color green --bold
            echo "Exiting Zeen's Kanban Board. Goodbye!"
            set_color normal
            break
        case TIP tip Tip
            echo "append --status or -s to get possible status return"
            echo "append --return-stats or -r to test return status"
        case '*'
            set_color red
            echo "Invalid choice '$choice'. Please enter 1, 2, 3, 4, 5, 6, 7, 8, 9, ALL, T, or X."
            set_color normal
    end
end
