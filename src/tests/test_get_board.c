#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../models.h"
#include "../features/get_board.h"

int main() {
    printf("========== TEST 1: Single-Threaded Search (< 100 items) ==========\n");
    Project *project1 = calloc(1, sizeof(Project));
    assert(project1 != NULL);

    // Add 5 boards (IDs "1", "2", "3", "4", "5")
    for (int i = 1; i <= 5; i++) {
        char title[100];
        snprintf(title, sizeof(title), "Board Number %d", i);
        addBoard(project1, title);
    }
    assert(project1->totalBoards == 5);

    // Test finding board "3"
    Board *b1 = findBoard(project1, "3");
    assert(b1 != NULL);
    assert(strcmp(b1->boardTitle, "Board Number 3") == 0);
    printf("Found board '3': %s\n", b1->boardTitle);

    // Test non-existent board "99"
    Board *b_null1 = findBoard(project1, "99");
    assert(b_null1 == NULL);
    printf("Non-existent board test passed!\n");
    free(project1);

    printf("\n========== TEST 2: Multi-Threaded Parallel Search (>= 100 items) ==========\n");
    Project *project2 = calloc(1, sizeof(Project));
    assert(project2 != NULL);

    // Add 250 boards (IDs "1" to "250")
    // Should trigger 3 worker threads: Thread 0 (0..99), Thread 1 (100..199), Thread 2 (200..249)
    for (int i = 1; i <= 250; i++) {
        char title[100];
        snprintf(title, sizeof(title), "Parallel Board %d", i);
        addBoard(project2, title);
    }
    assert(project2->totalBoards == 250);

    // Test finding item in Thread 0 range (e.g. ID "50")
    Board *t0_board = findBoard(project2, "50");
    assert(t0_board != NULL);
    assert(strcmp(t0_board->boardTitle, "Parallel Board 50") == 0);
    printf("Thread 0 Range Result: Found board '50' -> %s\n", t0_board->boardTitle);

    // Test finding item in Thread 1 range (e.g. ID "150")
    Board *t1_board = findBoard(project2, "150");
    assert(t1_board != NULL);
    assert(strcmp(t1_board->boardTitle, "Parallel Board 150") == 0);
    printf("Thread 1 Range Result: Found board '150' -> %s\n", t1_board->boardTitle);

    // Test finding item in Thread 2 range (e.g. ID "240")
    Board *t2_board = findBoard(project2, "240");
    assert(t2_board != NULL);
    assert(strcmp(t2_board->boardTitle, "Parallel Board 240") == 0);
    printf("Thread 2 Range Result: Found board '240' -> %s\n", t2_board->boardTitle);

    // Test non-existent board "999"
    Board *t_null = findBoard(project2, "999");
    assert(t_null == NULL);
    printf("Non-existent parallel search test passed!\n");

    free(project2);

    printf("\n✅ ALL MULTI-THREADING AND SEQUENTIAL SEARCH TESTS PASSED!\n");
    return 0;
}
