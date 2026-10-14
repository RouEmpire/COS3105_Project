#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "../models.h"
#include "../features/get_board.h"
#include "../features/get_task.h"
// #include "../features/update_task.h"

// Define global semaphore pointer used across features
sem_t *sem = NULL;

int main()
{
    printf("========== TEST 1: Update Task Title (Strict Status Preservation) ==========\n");

    // Initialize named semaphore for unit testing
    sem = sem_open("/store_lock_test", O_CREAT, 0644, 1);
    assert(sem != SEM_FAILED);

    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    // Create Board 1
    addBoard(project, "Engineering Sprint Board");
    Board *b1 = findBoard(project, "1");
    assert(b1 != NULL);

    // Populate Board 1 with 150 tasks (to test parallel search with >= 100 tasks)
    for (int t = 1; t <= 150; t++) {
        char title[64];
        snprintf(title, sizeof(title), "Sprint Task %d", t);
        addTask(b1, title);
    }
    assert(b1->totalTasks == 150);

    // Mark Task 1 as Finished (isFinish = 1) and Task 130 as Pending (isFinish = 0)
    b1->tasks[0].isFinish = 1;
    b1->tasks[129].isFinish = 0;

    // Test 1: Update Task "1" (Sequential search range, isFinish = 1)
    Task *t1 = findTask(b1, "1");
    assert(t1 != NULL);
    assert(strcmp(t1->taskTitle, "Sprint Task 1") == 0);
    assert(t1->isFinish == 1);

    // Perform update guarded by semaphore
    sem_wait(sem);
    strncpy(t1->taskTitle, "Write API Documentation", TASK_TITLE_SIZE - 1);
    t1->taskTitle[TASK_TITLE_SIZE - 1] = '\0';
    // CRITICAL: Ensure isFinish is NOT modified!
    saveAllTasks(project, "test_task_out.txt");
    sem_post(sem);

    assert(strcmp(t1->taskTitle, "Write API Documentation") == 0);
    assert(t1->isFinish == 1); // Status MUST still be 1!
    printf("Successfully updated Task 1 title to: '%s' (isFinish preserved as %d)\n",
           t1->taskTitle, t1->isFinish);

    // Test 2: Update Task "130" (Parallel search range >= 100, isFinish = 0)
    Task *t130 = findTask(b1, "130");
    assert(t130 != NULL);
    assert(strcmp(t130->taskTitle, "Sprint Task 130") == 0);
    assert(t130->isFinish == 0);

    sem_wait(sem);
    strncpy(t130->taskTitle, "Fix Memory Leak in Cache", TASK_TITLE_SIZE - 1);
    t130->taskTitle[TASK_TITLE_SIZE - 1] = '\0';
    // CRITICAL: Ensure isFinish is NOT modified!
    saveAllTasks(project, "test_task_out.txt");
    sem_post(sem);

    assert(strcmp(t130->taskTitle, "Fix Memory Leak in Cache") == 0);
    assert(t130->isFinish == 0); // Status MUST still be 0!
    printf("Successfully updated Task 130 title to: '%s' (isFinish preserved as %d)\n",
           t130->taskTitle, t130->isFinish);

    // Test 3: Search for non-existent Task "999" on Board 1
    Task *t999 = findTask(b1, "999");
    assert(t999 == NULL);
    printf("Correctly confirmed non-existent Task '999' returns NULL.\n");

    // Clean up
    remove("test_task_out.txt");
    sem_close(sem);
    sem_unlink("/store_lock_test");
    free(project);

    printf("\n✅ ALL UPDATE_TASK FEATURE TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
