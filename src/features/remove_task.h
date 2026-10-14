#ifndef REMOVE_TASK_H
#define REMOVE_TASK_H

#include "../server.h"
#include "get_board.h"
#include "get_task.h"

/* Soft-delete a task selected by board ID and task ID. */
void handleRemoveTask(CommandContext *ctx);

#endif /* REMOVE_TASK_H */
