#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../models.h"
#include "../features/add_board.h"
#include "../features/get_board.h"

// Define global semaphore pointer for unit test execution
sem_t *sem = NULL;

int main() {
    printf("========== TEST: ADD_BOARD Feature ==========\n");

    // Initialize named semaphore for test process
    sem = sem_open("/store_lock_test", O_CREAT, 0644, 1);
    assert(sem != SEM_FAILED);

    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    // 1. Add first board
    int res1 = addBoard(project, "My First Board");
    assert(res1 == 0);
    assert(project->totalBoards == 1);
    assert(strcmp(project->boards[0].boardTitle, "My First Board") == 0);
    assert(strcmp(project->boards[0].boardId, "1") == 0);
    printf("Successfully created Board 1: '%s'\n", project->boards[0].boardTitle);

    // 2. Add second board
    int res2 = addBoard(project, "Project Board 2");
    assert(res2 == 0);
    assert(project->totalBoards == 2);
    assert(strcmp(project->boards[1].boardTitle, "Project Board 2") == 0);
    assert(strcmp(project->boards[1].boardId, "2") == 0);
    printf("Successfully created Board 2: '%s'\n", project->boards[1].boardTitle);

    // 3. Verify lookup works for newly created boards
    Board *b2 = findBoard(project, "2");
    assert(b2 != NULL);
    assert(strcmp(b2->boardTitle, "Project Board 2") == 0);
    printf("Lookup verified Board 2 title: '%s'\n", b2->boardTitle);

    sem_close(sem);
    sem_unlink("/store_lock_test");
    free(project);

    printf("✅ ADD_BOARD FEATURE TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
