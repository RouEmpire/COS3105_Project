#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
// #include <string.h>
#include "../models.h"
#include "../features/get_all_data.h"

// Helper to prepare isolated test fixture data
static void setupTestFixture()
{
    FILE *bf = fopen(BOARD_FILENAME, "w");
    assert(bf != NULL);
    for (int i = 1; i <= 20; i++) {
        fprintf(bf, "%d,Parallel Board %d\n", i, i);
    }
    fclose(bf);

    FILE *tf = fopen(TASK_FILENAME, "w");
    assert(tf != NULL);
    for (int b = 1; b <= 20; b++) {
        for (int t = 1; t <= 5; t++) {
            fprintf(tf, "%d,%d,Task %d for Board %d,%d\n", b, t, t, b, t % 2);
        }
    }
    fclose(tf);
}

int main()
{
    printf("Setting up test fixture files (20 boards, 5 tasks each)...\n");
    setupTestFixture();

    printf("\n========== TEST 1: Thread Pool with Board Limit and Task Limit ==========\n");
    // Limit to 5 boards, max 3 tasks per board
    int boardLimit = 5;
    int taskLimit = 3;

    SharedProjectData *data1 = fetchAllDataParallel(boardLimit, taskLimit);
    assert(data1 != NULL);
    assert(data1->boardCount == 5);
    printf("Successfully retrieved %d boards with Thread Pool!\n", data1->boardCount);

    for (int i = 0; i < data1->boardCount; i++) {
        LoadedBoard *b = &data1->boards[i];
        printf("  -> Board #%s: '%s' (%d tasks loaded)\n", b->boardId, b->boardTitle, b->taskCount);
        assert(b->taskCount == 3);
        for (int t = 0; t < b->taskCount; t++) {
            printf("       Task [%s]: %s (isFinish: %d)\n",
                   b->tasks[t].taskId, b->tasks[t].taskTitle, b->tasks[t].isFinish);
        }
    }

    // Verify ordering (must be sorted 1, 2, 3, 4, 5)
    for (int i = 0; i < data1->boardCount - 1; i++) {
        int curId = atoi(data1->boards[i].boardId);
        int nextId = atoi(data1->boards[i+1].boardId);
        assert(curId <= nextId);
    }
    printf("Order verification passed: boards are sorted numerically.\n");
    freeSharedProjectData(data1);

    printf("\n========== TEST 2: Thread Pool with 10 Boards and 1 Task Each ==========\n");
    SharedProjectData *data2 = fetchAllDataParallel(10, 1);
    assert(data2 != NULL);
    assert(data2->boardCount == 10);
    for (int i = 0; i < data2->boardCount; i++) {
        assert(data2->boards[i].taskCount == 1);
    }
    printf("Retrieved 10 boards, exactly 1 task each.\n");
    freeSharedProjectData(data2);

    printf("\n✅ ALL THREAD POOL TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
