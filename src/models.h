#ifndef MODELS_H 
#define MODELS_H

#include "common.h"

#define BOARDID_SIZE 16
#define BOARD_TITLE_SIZE 500
#define TASKID_SIZE 16
#define TASK_TITLE_SIZE 500
#define MAX_TASK 500
#define MAX_BOARD 1000

#define BOARD_FILENAME "board.txt"
#define TASK_FILENAME "task.txt"

/* Exact on-disk representation of a soft-deleted board record. */
#define DELETED_BOARD_RECORD "---"

typedef struct task Task;

struct task {
    char taskId[TASKID_SIZE];
    char taskTitle[TASK_TITLE_SIZE];
    int isFinish;
    /* Non-zero when this array slot is a soft-deleted task tombstone. */
    int isDeleted;
};

typedef struct board Board;

struct board {
    char boardId[BOARDID_SIZE];
    char boardTitle[BOARD_TITLE_SIZE];
    struct task tasks[MAX_TASK];
    int totalTasks;
    /* Non-zero when this array slot is a soft-deleted board tombstone. */
    int isDeleted;
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