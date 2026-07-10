#include "ef_device.h"

#include <string.h>

static ef_device_t *s_registry_head = NULL;

ef_err_t ef_device_register(ef_device_t *dev, const char *name, const ef_device_ops_t *ops,
                            void *user_data)
{
    if (dev == NULL || name == NULL || ops == NULL) {
        return EF_ERR_PARAM;
    }
    if (strlen(name) >= EF_DEVICE_NAME_MAX) {
        return EF_ERR_PARAM;
    }
    if (ef_device_find(name) != NULL) {
        return EF_ERR_BUSY; /* 名字已被占用 */
    }

    memset(dev, 0, sizeof(*dev));
    strncpy(dev->name, name, EF_DEVICE_NAME_MAX - 1);
    dev->ops = ops;
    dev->user_data = user_data;
    dev->initialized = 0;

    dev->next = s_registry_head;
    s_registry_head = dev;

    if (ops->init != NULL) {
        return ops->init(dev);
    }
    dev->initialized = 1;
    return EF_OK;
}

ef_device_t *ef_device_find(const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    for (ef_device_t *dev = s_registry_head; dev != NULL; dev = dev->next) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
    }
    return NULL;
}

ef_err_t ef_device_open(ef_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL) {
        return EF_ERR_PARAM;
    }
    if (dev->ops->open == NULL) {
        return EF_OK; /* 设备不要求 open 语义，视为总是成功 */
    }
    return dev->ops->open(dev);
}

ef_err_t ef_device_close(ef_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL) {
        return EF_ERR_PARAM;
    }
    if (dev->ops->close == NULL) {
        return EF_OK;
    }
    return dev->ops->close(dev);
}

int32_t ef_device_read(ef_device_t *dev, void *buf, uint32_t len)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->read == NULL) {
        return (int32_t)EF_ERR_NOTSUPPORTED;
    }
    return dev->ops->read(dev, buf, len);
}

int32_t ef_device_write(ef_device_t *dev, const void *buf, uint32_t len)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->write == NULL) {
        return (int32_t)EF_ERR_NOTSUPPORTED;
    }
    return dev->ops->write(dev, buf, len);
}

ef_err_t ef_device_control(ef_device_t *dev, int32_t cmd, void *arg)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->control == NULL) {
        return EF_ERR_NOTSUPPORTED;
    }
    return dev->ops->control(dev, cmd, arg);
}

void ef_device_registry_reset(void)
{
    s_registry_head = NULL;
}
