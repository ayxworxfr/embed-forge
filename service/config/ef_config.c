#include "ef_config.h"

#include <string.h>

#include "ef_init.h"

#ifndef EF_CONFIG_MAX_ENTRIES
#define EF_CONFIG_MAX_ENTRIES 32
#endif

typedef enum {
    ENTRY_TYPE_I32,
    ENTRY_TYPE_STR,
} entry_type_t;

typedef struct {
    char key[EF_CONFIG_KEY_MAX];
    entry_type_t type;
    union {
        int32_t i32_value;
        char str_value[EF_CONFIG_STR_VALUE_MAX];
    } value;
    uint8_t used;
} config_entry_t;

static config_entry_t s_entries[EF_CONFIG_MAX_ENTRIES];

static config_entry_t *find_entry(const char *key)
{
    for (uint32_t i = 0; i < EF_CONFIG_MAX_ENTRIES; i++) {
        if (s_entries[i].used && strcmp(s_entries[i].key, key) == 0) {
            return &s_entries[i];
        }
    }
    return NULL;
}

static config_entry_t *find_or_alloc_entry(const char *key)
{
    config_entry_t *entry = find_entry(key);
    if (entry != NULL) {
        return entry;
    }
    for (uint32_t i = 0; i < EF_CONFIG_MAX_ENTRIES; i++) {
        if (!s_entries[i].used) {
            s_entries[i].used = 1;
            strncpy(s_entries[i].key, key, EF_CONFIG_KEY_MAX - 1);
            s_entries[i].key[EF_CONFIG_KEY_MAX - 1] = '\0';
            return &s_entries[i];
        }
    }
    return NULL;
}

ef_err_t ef_config_init(void)
{
    ef_config_reset();
    return ef_config_load();
}
EF_INIT_EXPORT(ef_config_init, EF_INIT_LEVEL_SERVICE);

ef_err_t ef_config_set_i32(const char *key, int32_t value)
{
    if (key == NULL || strlen(key) >= EF_CONFIG_KEY_MAX) {
        return EF_ERR_PARAM;
    }
    config_entry_t *entry = find_or_alloc_entry(key);
    if (entry == NULL) {
        return EF_ERR_NOMEM;
    }
    entry->type = ENTRY_TYPE_I32;
    entry->value.i32_value = value;
    return EF_OK;
}

int32_t ef_config_get_i32(const char *key, int32_t default_value)
{
    config_entry_t *entry = find_entry(key);
    if (entry == NULL || entry->type != ENTRY_TYPE_I32) {
        return default_value;
    }
    return entry->value.i32_value;
}

ef_err_t ef_config_set_str(const char *key, const char *value)
{
    if (key == NULL || value == NULL || strlen(key) >= EF_CONFIG_KEY_MAX ||
        strlen(value) >= EF_CONFIG_STR_VALUE_MAX) {
        return EF_ERR_PARAM;
    }
    config_entry_t *entry = find_or_alloc_entry(key);
    if (entry == NULL) {
        return EF_ERR_NOMEM;
    }
    entry->type = ENTRY_TYPE_STR;
    strncpy(entry->value.str_value, value, EF_CONFIG_STR_VALUE_MAX - 1);
    entry->value.str_value[EF_CONFIG_STR_VALUE_MAX - 1] = '\0';
    return EF_OK;
}

ef_err_t ef_config_get_str(const char *key, char *out_buf, uint32_t buf_size,
                           const char *default_value)
{
    if (out_buf == NULL || buf_size == 0) {
        return EF_ERR_PARAM;
    }

    config_entry_t *entry = find_entry(key);
    const char *src =
        (entry != NULL && entry->type == ENTRY_TYPE_STR) ? entry->value.str_value : default_value;
    if (src == NULL) {
        out_buf[0] = '\0';
        return EF_ERR_NOTFOUND;
    }

    strncpy(out_buf, src, buf_size - 1);
    out_buf[buf_size - 1] = '\0';

    return (entry != NULL && entry->type == ENTRY_TYPE_STR) ? EF_OK : EF_ERR_NOTFOUND;
}

ef_err_t ef_config_load(void)
{
    return EF_OK; /* RAM 版本：没有持久化介质，视为总是加载成功（空配置） */
}

ef_err_t ef_config_save(void)
{
    return EF_OK; /* RAM 版本：没有持久化介质，视为总是保存成功（实际不落盘） */
}

void ef_config_reset(void)
{
    memset(s_entries, 0, sizeof(s_entries));
}
