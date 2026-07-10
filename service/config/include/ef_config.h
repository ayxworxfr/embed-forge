/*
 * ef_config.h — 参数管理（KV 配置）
 *
 * v1 只做 RAM 版本，进程/固件重启后丢失。预留的扩展点：把
 * ef_config_load()/ef_config_save() 接上 Flash（比如集成 FlashDB 的
 * 思路做磨损均衡），app/service 层的 get/set 调用不需要改一行——
 * 这正是"可裁剪、可替换实现"的体现。
 */
#ifndef EF_CONFIG_H
#define EF_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

#define EF_CONFIG_KEY_MAX 24
#define EF_CONFIG_STR_VALUE_MAX 32

ef_err_t ef_config_init(void);

ef_err_t ef_config_set_i32(const char *key, int32_t value);
int32_t ef_config_get_i32(const char *key, int32_t default_value);

ef_err_t ef_config_set_str(const char *key, const char *value);
/** 返回 EF_OK 且把值拷贝进 out_buf；key 不存在时拷贝 default_value 并返回 EF_ERR_NOTFOUND */
ef_err_t ef_config_get_str(const char *key, char *out_buf, uint32_t buf_size,
                           const char *default_value);

/** 扩展点：当前是空实现（RAM 版本天然"加载成功"），留给 Flash 后端覆盖 */
ef_err_t ef_config_load(void);
ef_err_t ef_config_save(void);

/** 仅用于单元测试：清空所有配置项 */
void ef_config_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EF_CONFIG_H */
