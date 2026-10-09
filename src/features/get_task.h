#ifndef GET_TASK_H
#define GET_TASK_H

#include "../server.h"

/*
 * ============================================================================
 * FEATURE: GET_TASK / GET_TASKS
 * ============================================================================
 * Handles fetching individual task details or listing all tasks belonging
 * to a board.
 */

void handleGetTask(CommandContext *ctx);

#endif
