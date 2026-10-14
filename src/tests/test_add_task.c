#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../models.h"
#include "../features/get_board.h"
#include "../features/get_task.h"
// #include "../features/add_task.h"

int main()
{
    printf("========== TEST 1: Reusable Board Parallel Search (> 100 boards) ==========\n");
    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    // Create 150 boards (IDs "1" to "150")
    for (int i = 1; i <= 150; i++) {
        char title[64];
        snprintf(title, sizeof(title), "Classroom Board %d", i);
        addBoard(project, title);
    }
    assert(project->totalBoards == 150);

    // Parallel search for board "120" (in Thread 1's range [100..150))
    Board *b120 = findBoard(project, "120");
    assert(b120 != NULL);
    assert(strcmp(b120->boardTitle, "Classroom Board 120") == 0);
    printf("Found Board '120' using 1 thread per 100 boards: %s\n", b120->boardTitle);

    printf("\n========== TEST 2: Reusable Task Parallel Search (> 100 tasks) ==========\n");
    // Add 150 tasks to Board 120 (IDs "1" to "150")
    for (int t = 1; t <= 150; t++) {
        char taskTitle[64];
        snprintf(taskTitle, sizeof(taskTitle), "Homework Assignment %d", t);
        addTask(b120, taskTitle);
    }
    assert(b120->totalTasks == 150);

    // 1 thread per 100 tasks parallel search for "Homework Assignment 135" (Thread 1's range)
    Task *foundTask = findTaskByName(b120, "Homework Assignment 135");
    assert(foundTask != NULL);
    assert(strcmp(foundTask->taskId, "135") == 0);
    printf("Found Task by name using 1 thread per 100 tasks: Task ID %s -> %s\n",
           foundTask->taskId, foundTask->taskTitle);

    // Search for non-existent task
    Task *notFound = findTaskByName(b120, "Non-existent Task XYZ");
    assert(notFound == NULL);
    printf("Correctly determined 'Non-existent Task XYZ' is not on the board.\n");

    printf("\n========== TEST 3: Add Task Execution & Persistence ==========\n");
    // Add a brand new task (Task 151)
    int res = addTask(b120, "Final Exam Project");
    assert(res == 0);
    assert(b120->totalTasks == 151);

    // Re-verify that the newly added task can now be found
    Task *newTask = findTaskByName(b120, "Final Exam Project");
    assert(newTask != NULL);
    assert(strcmp(newTask->taskId, "151") == 0);
    printf("Successfully added and verified new task: ID %s -> %s\n",
           newTask->taskId, newTask->taskTitle);

    free(project);
    printf("\n✅ ALL ADD_TASK AND PARALLEL REUSABILITY TESTS PASSED!\n");
    return 0;
}
