#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "models.h"
#include "server.h"
#include "features/get_board.h"

int main() {
    printf("Testing handleGetBoard directly\n");

    Project *proj = calloc(1, sizeof(Project));
    addBoard(proj, "Hello World Board");

    CommandContext ctx = {
        .project = proj,
        .clientSocket = 1, // stdout for testing (not a real socket, but we test logic)
        .rawInput = "1",
        .commandName = "GET_BOARD"
    };

    Board *b = hasBoard(ctx.project, "1");
    assert(b != NULL);
    printf("Found Board Title: '%s'\n", b->boardTitle);
    assert(strcmp(b->boardTitle, "Hello World Board") == 0);

    free(proj);
    printf("Direct command test passed!\n");
    return 0;
}
