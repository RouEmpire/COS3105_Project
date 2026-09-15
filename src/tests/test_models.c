#include <stdio.h>
#include <stdlib.h>
#include "../models.h"
#include <assert.h>


int main(){
    printf("Start testing all models\n");

    printf("Testing Project\n");
    printf("Testing add board and task to Project\n");
    Project *project = calloc(1, sizeof(Project));
    if (project == NULL) {
        printf("calloc failed! sizeof(Project) = %zu\n", sizeof(Project));
        return 1;
    }
    addBoard(project, "Board 1");
    assert(project->totalBoards == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].boardId)==0); // it should not empty


    addTask(&project->boards[0], "Task 1");
    assert(project->boards[0].totalTasks == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId)==0); // it should not empty

    printf("Testing save board to file\n");
    saveBoard(project, BOARD_FILENAME);
    assert(fileExists(BOARD_FILENAME) == 0);

    printf("Testing save tasks to file\n");
    saveAllTasks(project, TASK_FILENAME);
    assert(fileExists(TASK_FILENAME) == 0);

    // 4. Clean up allocated memory
    free(project);

    printf("Testing load board and task to Project\n");
    project = calloc(1, sizeof(Project));
    if (project == NULL) {
        printf("calloc failed! sizeof(Project) = %zu\n", sizeof(Project));
        return 1;
    }
    
    printf("Testing load board from file\n");
    readBoard(project, BOARD_FILENAME);
    assert(project->totalBoards == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].boardId)==0); // it should not empty
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0); // it should be Board 1

    printf("Testing load task from file\n");
    readAllTasks(project, TASK_FILENAME);
    assert(project->boards[0].totalTasks == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId)==0); // it should not empty
    assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 1") == 0); // it should be Task 1

    free(project);

    printf("Testing load project\n");
    project = calloc(1, sizeof(Project));
    if (project == NULL) {
        printf("calloc failed! sizeof(Project) = %zu\n", sizeof(Project));
        return 1;
    }
    
    printf("Testing load board from file\n");
    readBoard(project, BOARD_FILENAME);
    assert(project->totalBoards == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].boardId)==0); // it should not empty
    assert(strcmp(project->boards[0].boardTitle, "Board 1") == 0); // it should be Board 1

    printf("Testing load task from file\n");
    readAllTasks(project, TASK_FILENAME);
    assert(project->boards[0].totalTasks == 1); // it should be 1
    assert(isEmptyOrSpace(project->boards[0].tasks[0].taskId)==0); // it should not empty
    assert(strcmp(project->boards[0].tasks[0].taskTitle, "Task 1") == 0); // it should be Task 1

    free(project);
    printf("All model tests passed successfully!\n");
    return 0;
}
