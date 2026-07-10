# embed-forge

一套通用的嵌入式软件分层架构脚手架，参考 [RT-Thread](https://github.com/RT-Thread/rt-thread) 的设备驱动框架思想，
用一份可编译、可跑单测、可烧录到真机的参考实现，把"分层架构"讲清楚——不是空谈原则，是能跑起来的代码。

当前参考硬件：**STM32 Nucleo-F411RE**（STM32F411RET6，Cortex-M4F）。换芯片只需要重写 `hal/` + `board/`，
`osal/`、`driver/`、`service/`、`app/` 不用动一行。

---

## 为什么是这个架构

```
┌──────────────────────────────────────────────────┐
│  App          业务逻辑：状态机、策略、流程            │  app/
├──────────────────────────────────────────────────┤
│  Service      功能组件：日志/配置/事件总线/CLI       │  service/
├──────────────────────────────────────────────────┤
│  OSAL         操作系统抽象：裸机/FreeRTOS/host可切   │  osal/
├──────────────────────────────────────────────────┤
│  Driver       设备驱动：面向功能的抽象接口            │  driver/
├──────────────────────────────────────────────────┤
│  HAL          硬件抽象：面向寄存器的直接封装          │  hal/
├──────────────────────────────────────────────────┤
│  Hardware                                        │  board/（BSP：引脚/时钟）
└──────────────────────────────────────────────────┘
```

**核心原则：上层可以调用下层，下层绝不依赖上层。** 每一层只通过约定好的接口（函数声明、回调、消息队列、发布订阅）
与其他层交互，不 `#include` 上层头文件。详细设计动机、每层的边界判断标准、三种层间通信机制的适用场景，
见 [docs/architecture.md](docs/architecture.md)——**新增代码前必读**。

---

## 目录结构

```
embed-forge/
├── app/                    # App 层：app_main.c（接线）+ app_led_blink.c（业务示例）
├── service/                # Service 层：log / event_bus / config / cli，每个独立静态库
├── osal/                   # OSAL 层：bare（裸机）/ freertos / host（PC 单测用 pthread）
├── driver/                 # Driver 层：ef_device 设备框架 + drv_led/drv_button/drv_uart_console
├── hal/                    # HAL 层：STM32F4 GPIO/UART 薄封装（面向寄存器）
├── board/nucleo_f411re/    # BSP：引脚映射、时钟配置、中断向量表、链接脚本、启动文件
├── lib/                    # 纯算法/数据结构，零硬件零 OS 依赖：ef_common（错误码/断言）、ringbuffer
├── test/                   # PC 单元测试（Unity）+ 硬件 mock，覆盖 lib/driver-device/service/app
├── cmake/                  # 工具链文件（host / arm-none-eabi）+ third_party 集成
├── third_party/            # git submodule：CMSIS、STM32F4xx HAL、FreeRTOS-Kernel、Unity
├── docs/                   # 架构设计文档
└── tools/                  # 脚手架/烧录脚本
```

---

## 快速开始

### 1. PC 单元测试（不需要任何硬件，也不需要 ARM 工具链）

```bash
make deps        # 首次克隆后先同步 third_party submodule
make host-test
# 等价于: cmake --preset host && cmake --build --preset host -j && ctest --preset host
```

已在 host 上实际跑过：7 个测试目标（`test_ringbuffer`/`test_ef_device`/`test_event_bus`/
`test_log`/`test_config`/`test_cli`/`test_app_led_blink`）全部通过。这条路径只依赖本机的
gcc/clang + CMake，用来验证 `lib/`、`driver/device`（设备框架核心）、`service/` 和 `app/` 的
业务逻辑，`hal/`、`board/`、真实的 `driver/drv_*.c` 不会被编译（它们依赖真实寄存器）。

### 2. 交叉编译真机固件（需要 `arm-none-eabi-gcc`）

```bash
make target-bare        # 或 make target-freertos
# 等价于: cmake --preset target-bare && cmake --build --preset target-bare -j
```

产物是 `build-target-bare/app/embed_forge.elf`（以及 `.hex` / `.bin`），用
`tools/flash.sh target-bare` 烧录（依赖 `st-flash` 或 `openocd`），或者自己用喜欢的工具烧。

> 本仓库当前没有 `arm-none-eabi-gcc` 环境验证过这条路径的实际编译结果，CMake 配置本身是对的
> （工具链文件、链接脚本、`EF_OSAL_BACKEND` 切换逻辑），但没有本机编译通过的实测证据，见
> [.github/workflows/ci.yml](.github/workflows/ci.yml) 的 `target-build` job 会在 CI 里验证这条路径。

`EF_OSAL_BACKEND` 是 CMake cache 变量，`bare`/`freertos` 二选一，**业务代码（`app/`、`driver/`、`service/`）
不用改一行**——这就是 OSAL 层存在的意义。

---

## 想学点东西，动手改改看

不知道从哪下手？看 [docs/demos.md](docs/demos.md)——从"不用硬件也能跑"到"新增完整驱动"
分了 8 个由浅入深的小 demo，每个都写了具体改哪些文件、怎么验证。

## 新增一个驱动/服务

不需要改 `app_main.c`。新驱动/服务的 `.c` 文件末尾写一行：

```c
EF_INIT_EXPORT(your_module_init, EF_INIT_LEVEL_DRIVER); // 或 _SERVICE / _APP
```

链接器会把它自动挂进初始化链（真机固件下由链接器 section 机制驱动；PC 单测下这个宏是空操作，
测试代码显式调用 `your_module_init()`）。具体步骤和 host 侧怎么写 mock，见
[docs/architecture.md](docs/architecture.md) 和 [CONTRIBUTING.md](CONTRIBUTING.md)。

---

## 依赖（git submodule，`third_party/`）

| 库 | 用途 |
|:---|:---|
| [CMSIS Core](https://github.com/STMicroelectronics/cmsis_core) | Cortex-M4 核心寄存器定义 |
| [CMSIS Device F4](https://github.com/STMicroelectronics/cmsis_device_f4) | STM32F4 外设寄存器定义/启动支持 |
| [STM32F4xx HAL Driver](https://github.com/STMicroelectronics/stm32f4xx_hal_driver) | ST 官方外设驱动库，`hal/` 层薄封装它 |
| [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) | `osal/freertos` 后端 |
| [Unity](https://github.com/ThrowTheSwitch/Unity) | PC 单元测试框架 |

首次克隆记得带 submodule：

```bash
git clone --recurse-submodules <repo-url>
# 或者克隆完之后：
make deps
# 等价于: git submodule update --init --recursive
```

---

## 已知限制

- 裸机 OSAL 后端没有真正的抢占式调度器，只支持"单一 app 任务 + 内部 while(1)"的超级循环用法；
  多任务并发场景请切到 `EF_OSAL_BACKEND=freertos`。
- `ef_log_write` 用静态缓冲区格式化，非重入；多任务并发打日志需要调用方自行加锁。
- `ef_config` 目前是纯 RAM 实现，重启丢失；`ef_config_load`/`save` 是预留的 Flash 持久化扩展点。

完整设计文档见 [docs/architecture.md](docs/architecture.md)，贡献前请看 [CONTRIBUTING.md](CONTRIBUTING.md)。
