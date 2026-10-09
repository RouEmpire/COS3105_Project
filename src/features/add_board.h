#ifndef ADD_BOARD_H
#define ADD_BOARD_H

#include "../server.h"

/*
 * ============================================================================
 * FEATURE: ADD_BOARD (Create Board)
 * ============================================================================
 * Handles creating a new Board in the Project and persisting it to file.
 * Demonstrates Inter-Process Synchronization using POSIX Semaphores (sem_wait / sem_post).
 */

void handleAddBoard(CommandContext *ctx);

#endif
