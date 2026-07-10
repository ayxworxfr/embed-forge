/*
 * ef_cli.h — 命令行 shell（参考 letter-shell 的命令自注册思路，做了
 * 大幅简化）
 *
 * 命令通过 EF_CLI_CMD_EXPORT 或 ef_cli_register() 注册进静态表，
 * ef_cli_exec_line() 负责分词+分发，和"从哪里拿到一行文本"完全解耦：
 * 既可以喂字节流（真实 UART，见 ef_cli_feed_byte），也可以直接喂一整
 * 行字符串（单元测试）。
 */
#ifndef EF_CLI_H
#define EF_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

#define EF_CLI_MAX_ARGC 8
#define EF_CLI_LINE_MAX 128

typedef int (*ef_cli_cmd_fn_t)(int argc, char **argv);
typedef void (*ef_cli_output_t)(const char *data, uint32_t len);

ef_err_t ef_cli_init(void);

/** 设置命令输出通道（回显、命令结果、help 列表都走这里） */
void ef_cli_set_output(ef_cli_output_t output);

ef_err_t ef_cli_register(const char *name, ef_cli_cmd_fn_t fn, const char *help);

/** 解析并执行一整行命令（空格分词），主要给单元测试/直接调用用 */
ef_err_t ef_cli_exec_line(const char *line);

/** 喂一个字节：内部按行缓冲，遇到 \r 或 \n 触发 ef_cli_exec_line */
void ef_cli_feed_byte(uint8_t byte);

/** 仅用于单元测试：清空命令表和行缓冲 */
void ef_cli_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EF_CLI_H */
