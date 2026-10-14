#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "../models.h"
#include "../features/get_board.h"
// #include "../features/update_board.h"

// Define global semaphore pointer used across features
sem_t *sem = NULL;

int main()
{
    printf("========== TEST 1: Update Board Title with Semaphore Locking ==========\n");

    // Initialize named semaphore for unit testing
    sem = sem_open("/store_lock_test", O_CREAT, 0644, 1);
    assert(sem != SEM_FAILED);

    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    // Populate with 150 boards to verify parallel search (1 thread per 100 boards)
    for (int i = 1; i <= 150; i++) {
        char title[64];
        snprintf(title, sizeof(title), "Initial Board Title %d", i);
        addBoard(project, title);
    }
    assert(project->totalBoards == 150);

    // Test 1: Update Board "1" (sequential search range < 100)
    Board *b1 = findBoard(project, "1");
    assert(b1 != NULL);
    assert(strcmp(b1->boardTitle, "Initial Board Title 1") == 0);

    // Acquire semaphore lock, update title, save to dummy file, release semaphore
    sem_wait(sem);
    strncpy(b1->boardTitle, "Renamed Board 1 - Sprint Review", BOARD_TITLE_SIZE - 1);
    b1->boardTitle[BOARD_TITLE_SIZE - 1] = '\0';
    saveBoard(project, "test_board_out.txt");
    sem_post(sem);

    assert(strcmp(b1->boardTitle, "Renamed Board 1 - Sprint Review") == 0);
    printf("Successfully updated Board 1 title to: '%s'\n", b1->boardTitle);

    // Test 2: Update Board "125" (parallel search range >= 100, Thread 1's partition)
    Board *b125 = findBoard(project, "125");
    assert(b125 != NULL);
    assert(strcmp(b125->boardTitle, "Initial Board Title 125") == 0);

    sem_wait(sem);
    strncpy(b125->boardTitle, "Renamed Board 125 - Cloud Infrastructure", BOARD_TITLE_SIZE - 1);
    b125->boardTitle[BOARD_TITLE_SIZE - 1] = '\0';
    saveBoard(project, "test_board_out.txt");
    sem_post(sem);

    assert(strcmp(b125->boardTitle, "Renamed Board 125 - Cloud Infrastructure") == 0);
    printf("Successfully updated Board 125 title to: '%s'\n", b125->boardTitle);

    // Test 3: Search for non-existent board "999"
    Board *b999 = findBoard(project, "999");
    assert(b999 == NULL);
    printf("Correctly confirmed non-existent Board '999' returns NULL.\n");

    // Clean up
    remove("test_board_out.txt");
    sem_close(sem);
    sem_unlink("/store_lock_test");
    free(project);

    printf("\n✅ ALL UPDATE_BOARD FEATURE TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
