#define _DEFAULT_SOURCE
#include "../../lib/rogueutil-1.0/rogueutil.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 3030
#define INPUT_CAPACITY 256
#define RESPONSE_CAPACITY 32768
#define MENU_COUNT 11
#define NO_COLOR (-1)

typedef struct MenuItem {
    const char *title;
    const char *hint;
    const char *command;
} MenuItem;

/* The menu uses two columns so WASD moves a visible selection cursor. */
static const MenuItem menu_items[MENU_COUNT] = {
    {"Get board", "Read board title", "GET_BOARD"},
    {"Get tasks", "List tasks on a board", "GET_TASKS"},
    {"Add board", "Create a board", "ADD_BOARD"},
    {"Add task", "Create task on a board", "ADD_TASK"},
    {"Update board", "Rename a board", "UPDATE_BOARD"},
    {"Update task", "Rename a task", "UPDATE_TASK"},
    {"Delete board", "Soft-delete a board", "DELETE_BOARD"},
    {"Delete task", "Soft-delete a task", "DELETE_TASK"},
    {"Toggle task", "Change done/pending", "SET_TASK_STATUS"},
    {"Get all data", "Fetch boards and tasks", "GET_ALL"},
    {"Quit", "Exit this interface", NULL}
};

static void draw_menu(int selected, const char *notice)
{
    cls();
    setColor(LIGHTCYAN);
    printf("+----------------------------------------------------------------------------+\n");
    printf("|                         KANBAN BOARD  /  TUI                               |\n");
    printf("+----------------------------------------------------------------------------+\n");
    resetColor();
    printf("Use W/A/S/D to move the cursor. Enter selects. Q or Esc quits.\n\n");

    for (int row = 0; row < 6; row++)
    {
        for (int column = 0; column < 2; column++)
        {
            int index = row * 2 + column;
            if (index >= MENU_COUNT)
                continue;

            int x = column == 0 ? 4 : 42;
            int y = 6 + row * 3;
            locate(x, y);
            if (index == selected)
            {
                setColor(WHITE);
                setBackgroundColor(BLUE);
                printf(" > %-28s ", menu_items[index].title);
                resetColor();
            }
            else
            {
                setColor(GREY);
                printf("   %-28s ", menu_items[index].title);
                resetColor();
            }
            locate(x + 3, y + 1);
            setColor(DARKGREY);
            printf("%-30s", menu_items[index].hint);
            resetColor();
        }
    }

    locate(1, 25);
    setColor(LIGHTCYAN);
    printf("Selected: %s", menu_items[selected].title);
    resetColor();
    if (notice != NULL && notice[0] != '\0')
    {
        locate(1, 27);
        setColor(YELLOW);
        printf("%s", notice);
        resetColor();
    }
    fflush(stdout);
}

/*
 * Read one line without switching away from RogueUtil's raw-key input model.
 * RogueUtil's getch() temporarily disables terminal echo, so characters are
 * echoed here and backspace is handled explicitly.
 */
static int read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length = 0;
    if (capacity == 0)
        return 0;

    buffer[0] = '\0';
    printf("\n%s", prompt);
    fflush(stdout);

    for (;;)
    {
        int key = getkey();
        if (key == KEY_ESCAPE)
        {
            printf("\nCancelled.\n");
            return 0;
        }
        if (key == KEY_ENTER || key == '\n')
        {
            buffer[length] = '\0';
            putchar('\n');
            return 1;
        }
        if (key == 8 || key == 127 || key == KEY_DELETE)
        {
            if (length > 0)
            {
                length--;
                buffer[length] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
        if (key >= 32 && key <= 126 && length + 1 < capacity)
        {
            buffer[length++] = (char)key;
            buffer[length] = '\0';
            putchar(key);
            fflush(stdout);
        }
    }
}

static int read_required(const char *prompt, char *buffer, size_t capacity)
{
    if (!read_line(prompt, buffer, capacity))
        return 0;
    if (buffer[0] == '\0')
    {
        printf("A value is required.\n");
        return 0;
    }
    if (strchr(buffer, ',') != NULL)
    {
        printf("Commas are reserved as protocol separators; please omit commas.\n");
        return 0;
    }
    return 1;
}

static int read_numeric(const char *prompt, char *buffer, size_t capacity)
{
    if (!read_required(prompt, buffer, capacity))
        return 0;
    for (size_t i = 0; buffer[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)buffer[i]))
        {
            printf("Please enter a numeric ID.\n");
            return 0;
        }
    }
    return 1;
}

static int append_argument(char *command, size_t capacity, const char *argument)
{
    size_t used = strlen(command);
    int written = snprintf(command + used, capacity - used, ",%s", argument);
    return written >= 0 && (size_t)written < capacity - used;
}

/* Build a protocol request by asking only for the fields required by the menu. */
static int build_command(int selection, char *command, size_t capacity)
{
    char first[INPUT_CAPACITY];
    char second[INPUT_CAPACITY];
    char third[INPUT_CAPACITY];

    snprintf(command, capacity, "%s", menu_items[selection].command);
    switch (selection)
    {
    case 0: /* GET_BOARD */
        if (!read_numeric("Board ID: ", first, sizeof(first))) return 0;
        return append_argument(command, capacity, first);
    case 1: /* GET_TASK or GET_TASKS */
        if (!read_numeric("Board ID: ", first, sizeof(first))) return 0;
        if (!append_argument(command, capacity, first)) return 0;
        if (!read_line("Task ID (press Enter to list all tasks): ", second, sizeof(second))) return 0;
        if (second[0] != '\0')
            snprintf(command, capacity, "GET_TASK,%s,%s", first, second);
        return 1;
    case 2: /* ADD_BOARD */
        if (!read_required("New board title: ", first, sizeof(first))) return 0;
        return append_argument(command, capacity, first);
    case 3: /* ADD_TASK */
        if (!read_numeric("Board ID: ", first, sizeof(first)) ||
            !read_required("New task title: ", second, sizeof(second))) return 0;
        return append_argument(command, capacity, first) &&
               append_argument(command, capacity, second);
    case 4: /* UPDATE_BOARD */
        if (!read_numeric("Board ID: ", first, sizeof(first)) ||
            !read_required("New board title: ", second, sizeof(second))) return 0;
        return append_argument(command, capacity, first) &&
               append_argument(command, capacity, second);
    case 5: /* UPDATE_TASK */
        if (!read_numeric("Board ID: ", first, sizeof(first)) ||
            !read_numeric("Task ID: ", second, sizeof(second)) ||
            !read_required("New task title: ", third, sizeof(third))) return 0;
        return append_argument(command, capacity, first) &&
               append_argument(command, capacity, second) &&
               append_argument(command, capacity, third);
    case 6: /* DELETE_BOARD */
        if (!read_numeric("Board ID to soft-delete: ", first, sizeof(first))) return 0;
        return append_argument(command, capacity, first);
    case 7: /* DELETE_TASK */
    case 8: /* SET_TASK_STATUS */
        if (!read_numeric("Board ID: ", first, sizeof(first)) ||
            !read_numeric("Task ID: ", second, sizeof(second))) return 0;
        return append_argument(command, capacity, first) &&
               append_argument(command, capacity, second);
    case 9: /* GET_ALL */
        if (!read_line("Board limit (0 for all): ", first, sizeof(first)) ||
            !read_line("Task limit per board (0 for all): ", second, sizeof(second))) return 0;
        if (first[0] == '\0') snprintf(first, sizeof(first), "0");
        if (second[0] == '\0') snprintf(second, sizeof(second), "0");
        return append_argument(command, capacity, first) &&
               append_argument(command, capacity, second);
    default:
        return 0;
    }
}

/* Connect to the existing TCP server and collect its complete response. */
static int send_command(const char *command, char *response, size_t capacity)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 0;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) != 1 ||
        connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) != 0)
    {
        fprintf(stderr, "Cannot connect to %s:%d: %s\n",
                SERVER_ADDRESS, SERVER_PORT, strerror(errno));
        close(sock);
        return 0;
    }

    char request[INPUT_CAPACITY * 4];
    int request_length = snprintf(request, sizeof(request), "%s\n", command);
    if (request_length < 0 || (size_t)request_length >= sizeof(request))
    {
        fprintf(stderr, "Request is too long.\n");
        close(sock);
        return 0;
    }

    size_t sent = 0;
    while (sent < (size_t)request_length)
    {
        ssize_t count = send(sock, request + sent, (size_t)request_length - sent, 0);
        if (count <= 0)
        {
            perror("send");
            close(sock);
            return 0;
        }
        sent += (size_t)count;
    }
    shutdown(sock, SHUT_WR);

    size_t used = 0;
    while (used + 1 < capacity)
    {
        ssize_t count = recv(sock, response + used, capacity - used - 1, 0);
        if (count == 0)
            break;
        if (count < 0)
        {
            if (errno == EINTR)
                continue;
            perror("recv");
            close(sock);
            return 0;
        }
        used += (size_t)count;
    }
    response[used] = '\0';
    close(sock);
    return used > 0;
}

static const char *status_label(int status)
{
    switch (status)
    {
    case 0: return "STATUS_OK";
    case 1: return "STATUS_FAIL";
    case 3: return "STATUS_UNKNOWN";
    case 200: return "STATUS_INVALID_ARGUMENTS";
    case 201: return "STATUS_BOARD_NOT_FOUND";
    case 202: return "STATUS_EMPTY_BOARD";
    case 203: return "STATUS_TASK_NOT_FOUND";
    case 204: return "STATUS_EMPTY_TASK";
    case 500: return "STATUS_SAVE_BOARD_FAIL";
    case 501: return "STATUS_ADD_BOARD_FAIL";
    case 502: return "STATUS_SAVE_TASK_FAIL";
    case 503: return "STATUS_ADD_TASK_FAIL";
    default: return "UNKNOWN_STATUS_CODE";
    }
}

static void show_result(const char *command, const char *response)
{
    char first_line[RESPONSE_CAPACITY];
    snprintf(first_line, sizeof(first_line), "%s", response);
    char *newline = strchr(first_line, '\n');
    if (newline != NULL)
        *newline = '\0';

    char *comma = strchr(first_line, ',');
    int status = comma != NULL ? atoi(comma + 1) : -1;

    cls();
    setColor(LIGHTCYAN);
    printf("REQUEST  > %s\n", command);
    resetColor();
    printf("RESPONSE >\n%s", response);
    if (response[0] != '\0' && response[strlen(response) - 1] != '\n')
        putchar('\n');

    setColor(status == 0 ? LIGHTGREEN : LIGHTRED);
    printf("\nRETURNED STATUS: %d (%s)\n", status,
           status >= 0 ? status_label(status) : "MISSING_STATUS");
    resetColor();
    printf("\nPress any key to return to the menu.");
    fflush(stdout);
    (void)getkey();
}

int main(void)
{
    int selected = 0;
    int running = 1;
    char notice[128] = "";

    setConsoleTitle("Kanban Board TUI");
    hidecursor();

    while (running)
    {
        draw_menu(selected, notice);
        notice[0] = '\0';
        int key = getkey();
        int row = selected / 2;
        int column = selected % 2;

        switch (tolower(key))
        {
        case 'w':
            if (row > 0) row--;
            break;
        case 's':
            if (row < 5 && row * 2 + column + 2 < MENU_COUNT) row++;
            break;
        case 'a':
            if (column > 0) column--;
            break;
        case 'd':
            if (column == 0 && row * 2 + 1 < MENU_COUNT) column++;
            break;
        case 'q':
        case KEY_ESCAPE:
            running = 0;
            continue;
        case KEY_ENTER:
        case '\n':
            if (menu_items[selected].command == NULL)
            {
                running = 0;
                continue;
            }
            else
            {
                char command[INPUT_CAPACITY * 4];
                char response[RESPONSE_CAPACITY];
                cls();
                printf("=== %s ===\n", menu_items[selected].title);
                if (!build_command(selected, command, sizeof(command)))
                {
                    snprintf(notice, sizeof(notice), "Request cancelled or invalid input.");
                    printf("\nPress any key to return to the menu.");
                    fflush(stdout);
                    (void)getkey();
                    break;
                }
                if (!send_command(command, response, sizeof(response)))
                {
                    snprintf(notice, sizeof(notice), "No response: is ./run_server.sh running?");
                    printf("\nPress any key to return to the menu.");
                    fflush(stdout);
                    (void)getkey();
                    break;
                }
                show_result(command, response);
                break;
            }
        default:
            break;
        }

        selected = row * 2 + column;
        if (selected >= MENU_COUNT)
            selected = MENU_COUNT - 1;
    }

    resetColor();
    cls();
    showcursor();
    puts("Kanban TUI closed.");
    return 0;
}
