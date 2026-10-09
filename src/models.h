#ifndef MODELS_H 
#define MODELS_H

#include "common.h"

#define BOARDID_SIZE 16
#define BOARD_TITLE_SIZE 500
#define TASKID_SIZE 16
#define TASK_TITLE_SIZE 500
#define MAX_TASK 100
#define MAX_BOARD 1000

#define BOARD_FILENAME "board.txt"
#define TASK_FILENAME "task.txt"




typedef struct task Task;

struct task {
    char taskId[TASKID_SIZE];
    char taskTitle[TASK_TITLE_SIZE];
    int isFinish;
};

typedef struct board Board;

struct board {
    char boardId[BOARDID_SIZE];
    char boardTitle[BOARD_TITLE_SIZE];
    struct task tasks[MAX_TASK]; 
    int totalTasks;
};

typedef struct project Project;

struct project {
    struct board boards[MAX_BOARD]; 
    int totalBoards;
};

int loadProject(Project *project, char *boardFileName, char *taskFileName);
int readBoard(Project *project, char *fileName);
int saveBoard(Project *project, char *fileName);
int readAllTasks(Project *project, char *fileName);
int saveAllTasks(Project *project, char *fileName);
int addBoard(Project *project, char *boardTitle);
int addTask(Board *board, char *taskTitle);
Board *hasBoard(Project *project, char *boardId);
Task *hasTask(Project *project, char *boardId, char *taskId);
#endif