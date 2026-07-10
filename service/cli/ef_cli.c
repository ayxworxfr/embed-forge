#include "ef_cli.h"

#include <stdio.h>
#include <string.h>

#include "ef_init.h"

#ifndef EF_CLI_MAX_COMMANDS
#define EF_CLI_MAX_COMMANDS 16
#endif
#define EF_CLI_CMD_NAME_MAX 16
#define EF_CLI_HELP_MAX 48

typedef struct {
    char name[EF_CLI_CMD_NAME_MAX];
    ef_cli_cmd_fn_t fn;
    char help[EF_CLI_HELP_MAX];
    uint8_t used;
} cli_command_t;

static cli_command_t s_commands[EF_CLI_MAX_COMMANDS];
static ef_cli_output_t s_output = NULL;

static char s_line_buf[EF_CLI_LINE_MAX];
static uint32_t s_line_len = 0;

static void output_str(const char *s)
{
    if (s_output != NULL) {
        s_output(s, (uint32_t)strlen(s));
    }
}

static int cmd_help(int argc, char **argv)
{
    EF_UNUSED(argc);
    EF_UNUSED(argv);
    char line[EF_CLI_CMD_NAME_MAX + EF_CLI_HELP_MAX + 8];
    for (uint32_t i = 0; i < EF_CLI_MAX_COMMANDS; i++) {
        if (s_commands[i].used) {
            snprintf(line, sizeof(line), "  %-10s %s\r\n", s_commands[i].name, s_commands[i].help);
            output_str(line);
        }
    }
    return 0;
}

ef_err_t ef_cli_init(void)
{
    ef_cli_reset();
    return ef_cli_register("help", cmd_help, "list all commands");
}
EF_INIT_EXPORT(ef_cli_init, EF_INIT_LEVEL_SERVICE);

void ef_cli_set_output(ef_cli_output_t output)
{
    s_output = output;
}

ef_err_t ef_cli_register(const char *name, ef_cli_cmd_fn_t fn, const char *help)
{
    if (name == NULL || fn == NULL || strlen(name) >= EF_CLI_CMD_NAME_MAX) {
        return EF_ERR_PARAM;
    }
    for (uint32_t i = 0; i < EF_CLI_MAX_COMMANDS; i++) {
        if (!s_commands[i].used) {
            s_commands[i].used = 1;
            strncpy(s_commands[i].name, name, EF_CLI_CMD_NAME_MAX - 1);
            s_commands[i].fn = fn;
            if (help != NULL) {
                strncpy(s_commands[i].help, help, EF_CLI_HELP_MAX - 1);
            } else {
                s_commands[i].help[0] = '\0';
            }
            return EF_OK;
        }
    }
    return EF_ERR_NOMEM;
}

static uint32_t tokenize(char *line, char **argv, uint32_t max_argc)
{
    uint32_t argc = 0;
    char *p = line;

    while (*p != '\0' && argc < max_argc) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }
    return argc;
}

ef_err_t ef_cli_exec_line(const char *line)
{
    if (line == NULL) {
        return EF_ERR_PARAM;
    }

    char buf[EF_CLI_LINE_MAX];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *argv[EF_CLI_MAX_ARGC];
    uint32_t argc = tokenize(buf, argv, EF_CLI_MAX_ARGC);
    if (argc == 0) {
        return EF_OK; /* 空行，静默忽略 */
    }

    for (uint32_t i = 0; i < EF_CLI_MAX_COMMANDS; i++) {
        if (s_commands[i].used && strcmp(s_commands[i].name, argv[0]) == 0) {
            s_commands[i].fn((int)argc, argv);
            return EF_OK;
        }
    }

    char err_line[EF_CLI_CMD_NAME_MAX + 32];
    snprintf(err_line, sizeof(err_line), "unknown command: %s\r\n", argv[0]);
    output_str(err_line);
    return EF_ERR_NOTFOUND;
}

void ef_cli_feed_byte(uint8_t byte)
{
    if (byte == '\r' || byte == '\n') {
        if (s_line_len > 0) {
            s_line_buf[s_line_len] = '\0';
            ef_cli_exec_line(s_line_buf);
            s_line_len = 0;
        }
        return;
    }

    if ((byte == 0x08 || byte == 0x7F) && s_line_len > 0) { /* 退格 */
        s_line_len--;
        return;
    }

    if (s_line_len < EF_CLI_LINE_MAX - 1) {
        s_line_buf[s_line_len++] = (char)byte;
    }
}

void ef_cli_reset(void)
{
    memset(s_commands, 0, sizeof(s_commands));
    s_line_len = 0;
}
