#ifndef REMOVE_BOARD_H
#define REMOVE_BOARD_H

#include "../server.h"
#include "get_board.h"

/*
 * Soft-delete a board selected by its ID.
 *
 * The handler does not compact the boards array because array position is part
 * of this program's ID-generation scheme.  Instead, it marks the board as
 * deleted and saveBoard() writes the corresponding board.txt record as "---".
 */
void handleRemoveBoard(CommandContext *ctx);

#endif /* REMOVE_BOARD_H */
