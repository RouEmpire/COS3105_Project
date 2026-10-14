#include "models.h"

int loadProject(Project *project, char *boardFileName, char *taskFileName){
    if(readBoard(project, boardFileName) == 1){
        return 1;
    }
    if(readAllTasks(project, taskFileName) == 1){
        return 1;
    }
    return 0;
}
int readBoard(Project *project, char *fileName){
    FILE *file_ptr = fopen(fileName, "r");

    if (file_ptr == NULL) {
        // File doesn't exist, let's create it!
        printf("File not found. Creating a new one...\n");
        file_ptr = fopen(fileName, "w+"); // w+ creates it and allows reading
    }

    if (file_ptr == NULL)
        return 1;

    char buffer[255];
    int i = 0;
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);
    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL && i < MAX_BOARD)
    {
        // 1. Remove the line ending before comparing or tokenizing the record.
        buffer[strcspn(buffer, "\r\n")] = '\0';

        /*
         * Keep a tombstone in the same array slot.  Keeping the slot prevents
         * an application restart from renumbering later board IDs, while the
         * flag makes normal board operations treat this record as deleted.
         */
        if (strcmp(buffer, DELETED_BOARD_RECORD) == 0)
        {
            project->boards[i].isDeleted = 1;
            i++;
            continue;
        }

        // 2. Tokenize an active record: boardId,boardTitle.
        char *boardId = strtok(buffer, ",");
        char *boardTitle = strtok(NULL, ",");

        // 3. Check tokens before storing a valid active board.
        if (boardId != NULL && boardTitle != NULL)
        {
            strcpy(project->boards[i].boardId, boardId);
            strcpy(project->boards[i].boardTitle, boardTitle);
            project->boards[i].isDeleted = 0;
            i++;
        }
    }
    project->totalBoards = i;
    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int saveBoard(Project *project,  char *fileName){
    FILE *file_ptr;

    file_ptr = fopen(fileName, "w");

    if (file_ptr == NULL)
    {
        printf("Error opening file!\n");
        return 1;
    }
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);
    Board *boards = project->boards;
    /*
     * Write only allocated board slots.  A deleted slot is intentionally a
     * single "---" line, as required by the soft-delete file format.
     */
    for (int i = 0; i < project->totalBoards; i++)
    {
        if (boards[i].isDeleted)
        {
            fprintf(file_ptr, "%s\n", DELETED_BOARD_RECORD);
        }
        else
        {
            fprintf(file_ptr, "%s,%s\n", boards[i].boardId, boards[i].boardTitle);
        }
    }
    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int readAllTasks(Project *project, char *fileName){
    FILE *file_ptr = fopen(fileName, "r");

    if (file_ptr == NULL) {
        printf("File not found. Creating a new one...\n");
        file_ptr = fopen(fileName, "w+");
    }

    if (file_ptr == NULL)
        return 1;

    char buffer[255];
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);

    while (fgets(buffer, sizeof(buffer), file_ptr) != NULL)
    {
        // 1. Remove the line ending before parsing this task record.
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // 2. The board ID is retained even for a deleted task tombstone.
        char *boardId = strtok(buffer, ",");
        char *taskId = strtok(NULL, ",");
        if (boardId == NULL || taskId == NULL)
            continue; // skip malformed records

        // 3. Find the active board to which this task record belongs.
        Board *targetBoard = NULL;
        for (int b = 0; b < project->totalBoards; b++)
        {
            if (!project->boards[b].isDeleted &&
                strcmp(project->boards[b].boardId, boardId) == 0)
            {
                targetBoard = &project->boards[b];
                break;
            }
        }

        if (targetBoard == NULL || targetBoard->totalTasks >= MAX_TASK)
            continue;

        // 4. Restore a "boardId,---" tombstone without renumbering later tasks.
        int i = targetBoard->totalTasks;
        if (strcmp(taskId, DELETED_BOARD_RECORD) == 0)
        {
            targetBoard->tasks[i].isDeleted = 1;
            targetBoard->totalTasks++;
            continue;
        }

        char *taskTitle = strtok(NULL, ",");
        char *isFinishStr = strtok(NULL, ",");
        if (taskTitle == NULL || isFinishStr == NULL)
            continue;

        strcpy(targetBoard->tasks[i].taskId, taskId);
        strcpy(targetBoard->tasks[i].taskTitle, taskTitle);
        targetBoard->tasks[i].isFinish = atoi(isFinishStr);
        targetBoard->tasks[i].isDeleted = 0;
        targetBoard->totalTasks++;
    }

    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int saveAllTasks(Project *project, char *fileName){
    FILE *file_ptr = fopen(fileName, "w");
    if (file_ptr == NULL) {
        printf("Error opening file!\n");
        return 1;
    }
    int fd = fileno(file_ptr);
    flock(fd, LOCK_EX);

    for (int b = 0; b < project->totalBoards; b++) {
        Board *board = &project->boards[b];

        /* Deleted boards must not keep producing orphaned task records. */
        if (board->isDeleted) {
            continue;
        }

        for (int i = 0; i < board->totalTasks; i++) {
            if (board->tasks[i].isDeleted) {
                /* Keep the parent ID, but erase all task-specific data. */
                fprintf(file_ptr, "%s,%s\n", board->boardId, DELETED_BOARD_RECORD);
            } else {
                fprintf(file_ptr, "%s,%s,%s,%d\n",
                        board->boardId,
                        board->tasks[i].taskId,
                        board->tasks[i].taskTitle,
                        board->tasks[i].isFinish);
            }
        }
    }

    flock(fd, LOCK_UN);
    fclose(file_ptr);
    return 0;
}
int addBoard(Project *project, char *boardTitle){
    
    snprintf(project->boards[project->totalBoards].boardId, 
            sizeof(project->boards[project->totalBoards].boardId), 
            "%d", project->totalBoards+1);
    strcpy(project->boards[project->totalBoards].boardTitle, boardTitle);
    project->totalBoards++;
    return 0;
}
int addTask(Board *board, char *taskTitle){
    snprintf(board->tasks[board->totalTasks].taskId, sizeof(board->tasks[board->totalTasks].taskId), "%d", board->totalTasks + 1);
    strcpy(board->tasks[board->totalTasks].taskTitle, taskTitle);
    board->totalTasks++;
    return 0;
}
Board *hasBoard(Project *project,char *boardId){
    Board *board = NULL;
    if(project == NULL || boardId==NULL) return NULL;
    for (int i = 0; i < project->totalBoards; i++)
    {
        if (!project->boards[i].isDeleted &&
            strcmp(project->boards[i].boardId, boardId) == 0)
        {
            board = &project->boards[i];
            break;
        }
    }
    return board;
}
Task *hasTask(Project *project, char *boardId, char *taskId){
    Board *board = hasBoard(project, boardId);
    if(board==NULL || taskId == NULL) return NULL;
    Task *task = NULL;
    for (int i=0;i < board->totalTasks; i++){
        if (strcmp(board->tasks[i].taskId, taskId) == 0){
            task = & board->tasks[i];
            break;
        }
    }
    return task;
}