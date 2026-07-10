/*
 * ef_device.h — 设备框架核心（对齐 RT-Thread struct rt_device + ops）
 *
 * 应用层统一通过 ef_device_find("led0") + ef_device_read/write 访问
 * 具体外设，完全不感知底层是 STM32 还是别的芯片。新驱动只需要实现一份
 * ef_device_ops_t 并调用 ef_device_register() 挂进注册表。
 *
 * 零动态分配：注册表是静态链表，节点内存由调用者提供（通常是驱动的
 * 静态全局变量），符合嵌入式"不在运行期 malloc"的惯例。
 */
#ifndef EF_DEVICE_H
#define EF_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ef_common.h"

#define EF_DEVICE_NAME_MAX 16

typedef struct ef_device ef_device_t;

typedef struct {
    ef_err_t (*init)(ef_device_t *dev);
    ef_err_t (*open)(ef_device_t *dev);
    ef_err_t (*close)(ef_device_t *dev);
    int32_t (*read)(ef_device_t *dev, void *buf, uint32_t len);
    int32_t (*write)(ef_device_t *dev, const void *buf, uint32_t len);
    ef_err_t (*control)(ef_device_t *dev, int32_t cmd, void *arg);
} ef_device_ops_t;

struct ef_device {
    char name[EF_DEVICE_NAME_MAX];
    const ef_device_ops_t *ops;
    void *user_data; /* 驱动私有状态，由具体驱动自行解释 */
    uint8_t initialized;
    struct ef_device *next; /* 内部链表指针，调用者不要动 */
};

/** 注册设备。node 由调用者提供（通常是驱动文件里的 static ef_device_t 实例） */
ef_err_t ef_device_register(ef_device_t *dev, const char *name, const ef_device_ops_t *ops,
                            void *user_data);

/** 按名字查找设备，未找到返回 NULL */
ef_device_t *ef_device_find(const char *name);

ef_err_t ef_device_open(ef_device_t *dev);
ef_err_t ef_device_close(ef_device_t *dev);
int32_t ef_device_read(ef_device_t *dev, void *buf, uint32_t len);
int32_t ef_device_write(ef_device_t *dev, const void *buf, uint32_t len);
ef_err_t ef_device_control(ef_device_t *dev, int32_t cmd, void *arg);

/** 仅用于单元测试：清空注册表，避免测试用例之间互相污染 */
void ef_device_registry_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* EF_DEVICE_H */
