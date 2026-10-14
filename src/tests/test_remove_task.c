#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../features/get_task.h"
#include "../models.h"

/*
 * A task tombstone retains its board ID so the loader can restore the deleted
 * array slot.  This prevents task 3 from being renumbered to task 2 after a
 * restart, while findTask() continues to hide the removed task.
 */
int main(void)
{
    const char *testFile = "test_remove_task.txt";
    Project *project = calloc(1, sizeof(Project));
    assert(project != NULL);

    addBoard(project, "Test board");
    addTask(&project->boards[0], "Keep first task");
    addTask(&project->boards[0], "Delete this task");
    addTask(&project->boards[0], "Keep third task");

    Task *deletedTask = findTask(&project->boards[0], "2");
    assert(deletedTask != NULL);
    deletedTask->isDeleted = 1;
    assert(saveAllTasks(project, (char *)testFile) == 0);
    free(project);

    FILE *file = fopen(testFile, "r");
    assert(file != NULL);
    char line[64];
    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(line, "1,1,Keep first task,0\n") == 0);
    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(line, "1,---\n") == 0);
    fclose(file);

    project = calloc(1, sizeof(Project));
    assert(project != NULL);
    addBoard(project, "Test board");
    assert(readAllTasks(project, (char *)testFile) == 0);
    assert(project->boards[0].totalTasks == 3);
    assert(project->boards[0].tasks[1].isDeleted != 0);
    assert(findTask(&project->boards[0], "2") == NULL);
    assert(findTask(&project->boards[0], "3") != NULL);

    free(project);
    remove(testFile);
    puts("Soft-delete task persistence test passed.");
    return 0;
}
