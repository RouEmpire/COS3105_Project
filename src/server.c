#include "server.h"
#include "features/add_board.h"
#include "features/add_task.h"
#include "features/get_board.h"
#include "features/get_task.h"
#include "features/get_all_data.h"
#include "features/update_board.h"
#include "features/update_task.h"
#include "features/set_task_status.h"
#include <signal.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>    /* For O_CREAT, O_RDWR constants */
#include <sys/stat.h> /* For mode constants like 0644 */

SOCKET socket_listen = -1;
Project *project = NULL;
sem_t *sem = NULL;

int handleClient(SOCKET socket_client, char *read)
{
  // printf("line: %s\n", read);
  char response[MESSAGE_SIZE];
  char *saveptr;
  char *commandName = strtok_r(read, ",\n\r", &saveptr);

  if (commandName == NULL)
    return -1;

  CommandContext ctx = {
      .project = project,
      .clientSocket = socket_client,
      .rawInput = saveptr,
      .commandName = commandName,
  };

  CommandEntry commandTable[] = {
      {COMMAND_ADD_BOARD, handleAddBoard},
      {COMMAND_ADD_TASK, handleAddTask},
      {COMMAND_GET_BOARD, handleGetBoard},
      {COMMAND_GET_BOARDS, handleGetBoard},
      {COMMAND_GET_TASK, handleGetTask},
      {COMMAND_GET_TASKS, handleGetTask},
      {COMMAND_GET_ALL, handleGetAllData},
      {COMMAND_GET_ALL_DATA, handleGetAllData},
      {COMMAND_UPDATE_BOARD, handleUpdateBoard},
      {COMMAND_UPDATE_TASK, handleUpdateTask},
      {COMMAND_SET_TASK_STATUS, handleSetTaskStatus},
      {NULL, NULL} // Sentinel to mark the end
  };

  for (int i = 0; commandTable[i].commandName != NULL; i++)
  {
    if (strcmp(commandName, commandTable[i].commandName) == 0)
    {
      // Found the command! Execute its function.
      printf("Found command: %s\n", commandName);
      commandTable[i].handler(&ctx);
      return 0;
    }
  }
  snprintf(response, sizeof(response), "%s%s%d\n", COMMAND_UNKNOWN, COMMAND_SEPARATOR, STATUS_UNKNOWN);
  send(socket_client, response, strlen(response), 0);
  CLOSESOCKET(socket_client);
  return -1;
}

void handle_shutdown(int sig)
{
  if (sig == SIGINT)
  {
    printf("\nCaught Ctrl+C! Cleaning up...\n");
  }
  else if (sig == SIGTERM)
  {
    printf("\nTermination request received...\n");
  }
  // To prevent an infinite loop of signals
  signal(SIGINT, SIG_IGN);

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);
  socket_listen = -1;

  printf("Cleaning up semaphore...\n");
  sem_close(sem);
  sem_unlink("/store_lock");

  // 2. Kill all child processes in the group
  // Passing 0 as the PID sends the signal to everyone in the process group
  kill(0, SIGTERM);

  exit(0);
}

int setup(char *port)
{
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // init semaphore
  sem = sem_open("/store_lock", O_CREAT, 0644, 1);

  // Init project using shared memory (mmap)
  project = mmap(NULL, sizeof(Project), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  loadProject(project, BOARD_FILENAME, TASK_FILENAME);

  struct addrinfo *bind_address;
  getaddrinfo(0, port, &hints, &bind_address);

  printf("Creating socket...\n");

  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen))
  {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  // Allow socket address reuse
  int yes = 1;
  if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) <
      0)
  {
    fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Binding socket to local address...\n");
  printf("Binding to port: %s\n", SERVER_PORT);
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
  {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }
  freeaddrinfo(bind_address);

  printf("Listening...\n");
  if (listen(socket_listen, 10) < 0)
  {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
  }

  printf("Waiting for connections...\n");
  while (1)
  {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client =
        accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
    if (!ISVALIDSOCKET(socket_client))
    {
      // If we are shutting down, don't print an error
      if (socket_listen == -1)
      {
        return 0;
      }
      fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
      return 1;
    }

    char address_buffer[100];
    getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
                sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("New connection from %s\n", address_buffer);

    int pid = fork();
    if (pid == 0)
    { // child process
      CLOSESOCKET(socket_listen);

      while (1)
      {
        char read[1024];
        int bytes_received = recv(socket_client, read, 1023, 0);
        if (bytes_received < 1)
        {
          printf("Client disconnected.\n");

          CLOSESOCKET(socket_client);
          exit(0);
        }
        read[bytes_received] = '\0';

        char *line_saveptr;
        char *line = strtok_r(read, "\r\n", &line_saveptr);
        while (line != NULL)
        {
          if (!isEmptyOrSpace(line))
          {
            char line_copy[1024];
            strncpy(line_copy, line, sizeof(line_copy) - 1);
            line_copy[sizeof(line_copy) - 1] = '\0';
            handleClient(socket_client, line_copy);
          }
          line = strtok_r(NULL, "\r\n", &line_saveptr);
        }

        // Close socket and exit child process after serving request
        CLOSESOCKET(socket_client);
        exit(0);
      }
    }
    CLOSESOCKET(socket_client);
  } // while(1)

  printf("Finished.\n");
  return 0;
}

int main()
{

  signal(SIGINT, handle_shutdown);  // Catches CTRL+C
  signal(SIGTERM, handle_shutdown); // Catches kill command
  if (setup(SERVER_PORT) != 0)
  {
      fprintf(stderr, "Server setup failed. Is another server process already running on port %s?\n", SERVER_PORT);
      exit(1);
  }

  return 0;
}
