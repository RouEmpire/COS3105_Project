#ifndef SERVER_H 
#define SERVER_H


#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <netdb.h>
#include <semaphore.h>
#include "models.h"
#include "common.h"
#include "protocol.h"

extern sem_t *sem;


#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#define SERVER_PORT "3030"
#define MESSAGE_SIZE 2000

typedef struct CommandContext {
    Project *project;
    SOCKET clientSocket;
    char *rawInput;
    const char *commandName;
} CommandContext;

// Define function pointer CommandHandler
typedef void (*CommandHandler)(CommandContext *ctx);

typedef struct {
    char *commandName;
    CommandHandler handler;
} CommandEntry;


// Server Handlers
void handleAddBoard(CommandContext *ctx);
void handleAddTask(CommandContext *ctx);
void handleGetBoard(CommandContext *ctx);
void handleGetTask(CommandContext *ctx);
void handleGetAllData(CommandContext *ctx);
void handleUpdateBoard(CommandContext *ctx);
void handleUpdateTask(CommandContext *ctx);


// Common
int handleClient(SOCKET socket_client,char *read);
void handle_shutdown(int sig);
int setup(char *port);
#endif