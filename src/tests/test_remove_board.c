#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../features/get_board.h"
#include "../models.h"

/*
 * This regression test focuses on the data contract used by DELETE_BOARD:
 * the on-disk record must be exactly "---" and must remain a tombstone after
 * a project restart.  The handler itself uses findBoard() and saveBoard(), so
 * testing those two layers verifies the permanent soft-delete behaviour.
 */
int main(void)
{
    const char *testFile = "test_remove_board.txt";
    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    addBoard(project, "Keep first board");
    addBoard(project, "Delete this board");
    addBoard(project, "Keep third board");

    /* This is the same in-memory state change made by handleRemoveBoard(). */
    Board *deletedBoard = findBoard(project, "2");
    assert(deletedBoard != NULL);
    deletedBoard->isDeleted = 1;
    assert(saveBoard(project, (char *)testFile) == 0);
    free(project);

    FILE *file = fopen(testFile, "r");
    assert(file != NULL);
    char line[64];
    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(line, "1,Keep first board\n") == 0);
    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(line, "---\n") == 0);
    fclose(file);

    /* Reloading must preserve the deleted slot and hide it from normal lookup. */
    project = calloc(1, sizeof(Project));
    assert(project != NULL);
    assert(readBoard(project, (char *)testFile) == 0);
    assert(project->totalBoards == 3);
    assert(project->boards[1].isDeleted != 0);
    assert(findBoard(project, "2") == NULL);
    assert(findBoard(project, "3") != NULL);

    free(project);
    remove(testFile);
    puts("Soft-delete persistence test passed.");
    return 0;
}
