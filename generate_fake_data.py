#!/usr/bin/env python3
"""
generate_fake_data.py
Script to generate fake Kanban boards and tasks for testing and benchmarking.
Defaults to generating 1,000 boards and 300 tasks per board (300,000 total tasks).
"""

import os
import sys
import argparse
import random
import time

# Diverse sample themes and verbs for realistic fake names
BOARD_DOMAINS = [
    "Sprint Planning", "Feature Roadmap", "Bug Tracker", "DevOps Pipeline",
    "Security Audit", "API Redesign", "Cloud Migration", "Database Tuning",
    "Frontend UX", "Mobile App Core", "Machine Learning Lab", "Network Infra",
    "Code Refactoring", "Microservices", "Release Engineering"
]

TASK_VERBS = [
    "Implement", "Refactor", "Optimize", "Debug", "Design",
    "Review", "Document", "Audit", "Test", "Benchmark",
    "Configure", "Deploy", "Investigate", "Integrate", "Validate"
]

TASK_OBJECTS = [
    "authentication handler", "connection pooling", "thread pool worker",
    "TCP socket buffer", "memory allocation leak", "mutex synchronization",
    "POSIX shared memory", "CRUD endpoint logic", "data ingestion script",
    "caching layer", "unit test suite", "asynchronous event loop",
    "database indexing", "signal handler", "IPC message queue"
]


def generate_data(num_boards: int, tasks_per_board: int, board_file: str, task_file: str):
    print(f"Generating {num_boards:,} boards and {tasks_per_board:,} tasks per board...")
    print(f"Total tasks to generate: {num_boards * tasks_per_board:,}")
    start_time = time.time()

    # 1. Generate boards
    print(f"Writing boards to '{board_file}'...")
    with open(board_file, "w", encoding="utf-8") as bf:
        for b_id in range(1, num_boards + 1):
            domain = BOARD_DOMAINS[(b_id - 1) % len(BOARD_DOMAINS)]
            title = f"Board {b_id} - {domain}"
            bf.write(f"{b_id},{title}\n")

    # 2. Generate tasks
    print(f"Writing tasks to '{task_file}'...")
    total_tasks = 0
    with open(task_file, "w", encoding="utf-8") as tf:
        for b_id in range(1, num_boards + 1):
            for t_id in range(1, tasks_per_board + 1):
                verb = TASK_VERBS[(b_id + t_id) % len(TASK_VERBS)]
                obj = TASK_OBJECTS[(b_id * 3 + t_id) % len(TASK_OBJECTS)]
                task_title = f"{verb} {obj} #{t_id}"
                # Set status: 1 (finished) or 0 (in-progress)
                is_finish = 1 if (t_id % 3 == 0) else 0
                tf.write(f"{b_id},{t_id},{task_title},{is_finish}\n")
                total_tasks += 1

    elapsed = time.time() - start_time
    board_size_mb = os.path.getsize(board_file) / (1024 * 1024)
    task_size_mb = os.path.getsize(task_file) / (1024 * 1024)

    print("\nGeneration Completed Successfully!")
    print(f"  - Boards generated: {num_boards:,} ({board_size_mb:.2f} MB in '{board_file}')")
    print(f"  - Tasks generated:  {total_tasks:,} ({task_size_mb:.2f} MB in '{task_file}')")
    print(f"  - Time elapsed:     {elapsed:.2f} seconds")


def main():
    parser = argparse.ArgumentParser(
        description="Generate fake Kanban boards and tasks for testing."
    )
    parser.add_argument(
        "-b", "--boards",
        type=int,
        default=1000,
        help="Number of boards to generate (default: 1000)"
    )
    parser.add_argument(
        "-t", "--tasks",
        type=int,
        default=300,
        help="Number of tasks per board (default: 300)"
    )
    parser.add_argument(
        "--board-file",
        type=str,
        default="board.txt",
        help="Target output file for boards (default: board.txt)"
    )
    parser.add_argument(
        "--task-file",
        type=str,
        default="task.txt",
        help="Target output file for tasks (default: task.txt)"
    )

    args = parser.parse_args()
    generate_data(args.boards, args.tasks, args.board_file, args.task_file)


if __name__ == "__main__":
    main()
