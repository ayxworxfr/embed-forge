# embed-forge 架构设计文档

本文档是 **新增代码前必读** 的设计依据。README.md 讲"怎么用"，这份文档讲"为什么这么设计、
边界在哪、新代码应该往哪放"。评审 PR 时，任何违反这里原则的改动都应该被打回。

---

## 1. 分层与依赖方向

```
App        (app/)                 业务逻辑：状态机、策略、流程
Service    (service/)             功能组件：log / event_bus / config / cli
OSAL       (osal/)                操作系统抽象：bare / freertos / host
Driver     (driver/)              设备驱动：面向功能的抽象接口
HAL        (hal/)                 硬件抽象：面向寄存器的直接封装
Hardware   (board/ = BSP)         引脚映射、时钟配置、启动文件、链接脚本
```

**唯一的硬性规则：上层可以 `#include` 并调用下层的头文件，下层永远不 `#include` 上层的头文件。**

判断一段代码该放哪一层，用这张表自查：

| 问题 | HAL | Driver | Service | App |
|:---|:---|:---|:---|:---|
| 出现寄存器/`GPIO_PIN_x`/`USART2` 这类芯片专属符号？ | ✅ 只能这里 | ❌ | ❌ | ❌ |
| 描述的是"一个具体外设的完整行为"（LED/按键/UART console）？ | ❌ | ✅ | ❌ | ❌ |
| 是不依赖具体设备、可独立编译测试的通用能力（日志/配置/CLI/事件总线）？ | ❌ | ❌ | ✅ | ❌ |
| 是业务流程/状态机，只调用下层语义化接口？ | ❌ | ❌ | ❌ | ✅ |

`lib/`（`ef_common`、`ef_ringbuffer`）是特例：零硬件、零 OS 依赖的纯算法/数据结构，
被所有层复用，不算在这条纵向依赖链里。

---

## 2. 模块地图

```
lib/
├── ef_common/     错误码 ef_err_t、EF_ASSERT、EF_MIN/MAX/ARRAY_SIZE —— 全项目的地基
└── ringbuffer/    单生产者/单消费者环形缓冲区，中断安全

osal/
├── include/osal.h         统一接口：mutex / queue / thread / delay / critical section
├── bare/osal_bare.c       裸机后端：协作式轮转，关中断实现 mutex
├── freertos/osal_freertos.c  FreeRTOS 后端：直接映射到 FreeRTOS API
└── host/osal_host.c       PC 单测后端：pthread 模拟

hal/
├── include/hal_gpio.h, hal_uart.h   芯片无关接口声明
└── stm32f4xx/             STM32F4 实现，包一层 ST 官方 HAL

board/nucleo_f411re/       BSP：board_config.h（引脚）、board_init.c（时钟+初始化）、
                            stm32f4xx_it.c（中断向量）、*.ld（链接脚本）、startup_*.s

driver/
├── device/                ef_device + ops 表（设备框架核心）+ ef_init（自动注册机制）
├── drv_led.c/.h
├── drv_button.c/.h
└── drv_uart_console.c/.h

service/
├── log/          分级日志，可插拔输出 sink
├── event_bus/    发布订阅（topic 字符串精确匹配）
├── config/       RAM KV 配置存储
└── cli/          命令行 shell，命令自注册

app/
├── app_main.c        固件入口，只做"接线"，不含业务逻辑
└── app_led_blink.c/.h  业务示例：按键 → 事件总线 → LED + 日志 + CLI

test/
├── host/         Unity 单测，覆盖 lib / driver-device / service / app
└── mocks/        硬件 mock（如 drv_led_mock），让 App 层业务代码可以在 PC 上跑
```

---

## 3. 设备框架：`ef_device`

对齐 RT-Thread 的 `struct rt_device`。核心思路：**驱动把自己注册成一个 `ef_device_t`，
应用层只认设备名字，不认具体驱动实现。**

```c
// 定义在 driver/device/ef_device.h
typedef struct {
    ef_err_t (*init)(ef_device_t *dev);
    ef_err_t (*open)(ef_device_t *dev);
    ef_err_t (*close)(ef_device_t *dev);
    int32_t  (*read)(ef_device_t *dev, void *buf, uint32_t len);
    int32_t  (*write)(ef_device_t *dev, const void *buf, uint32_t len);
    ef_err_t (*control)(ef_device_t *dev, int32_t cmd, void *arg);
} ef_device_ops_t;
```

新驱动的标准写法：

1. 静态分配一个 `ef_device_t` 实例（禁止 `malloc`）。
2. 实现一份 `ef_device_ops_t`（只实现自己支持的操作，其余留 `NULL`——`ef_device_read/write`
   在 `ops` 里对应函数为 `NULL` 时统一返回 `EF_ERR_NOTSUPPORTED`，调用方不用 `if` 判断能力）。
3. 调用 `ef_device_register(&dev, "name", &ops, user_data)`。

上层用 `ef_device_find("led0")` 拿到设备指针，之后统一走 `ef_device_read/write/control`。
好处：换一个 LED 驱动实现（比如从 GPIO 换成 PWM 呼吸灯），应用层代码一行不改。

---

## 4. 模块自动注册：`EF_INIT_EXPORT`

对齐 RT-Thread 的 `INIT_EXPORT`。**新增一个驱动/服务模块，不需要去改 `app_main.c` 手动调用初始化函数**，
在模块 `.c` 文件末尾写一行即可：

```c
EF_INIT_EXPORT(drv_led_init, EF_INIT_LEVEL_DRIVER);
```

四个级别（`EF_INIT_LEVEL_BOARD` < `DRIVER` < `SERVICE` < `APP`），决定初始化顺序——board 先于
driver，driver 先于 service，以此类推，因为上层初始化经常依赖下层已经就位（比如 `drv_uart_console_init`
依赖 `board_init` 已经配好 UART 的 GPIO/时钟）。

**实现机制**：链接器把每个 `EF_INIT_EXPORT` 生成的函数指针放进 `.ef_init.<level>` 段，链接脚本
（`board/*/*.ld`）按段名字典序排列，`ef_init_run_all()`（`driver/device/ef_init.c`）遍历
`[__ef_init_start, __ef_init_end)` 依次调用。

**这个机制只在真机固件（`CMAKE_CROSSCOMPILING`）下生效**，由根 `CMakeLists.txt` 定义
`EF_ENABLE_AUTO_INIT` 编译宏驱动。PC 单元测试下这个宏不定义，`EF_INIT_EXPORT` 退化为空操作
（一行没有副作用的 `typedef`），**测试代码必须显式调用各模块的 `xxx_init()`**——host 平台的目标
文件格式（PE/COFF、Mach-O）不保证支持"自定义 section + 起止符号"这套 ELF 惯用技巧，所以故意不在
host 上用它，避免"链接器段魔法在非 ELF 平台上到底靠不靠谱"这种不确定性。

---

## 5. 层间通信三种机制

### 5.1 回调（Callback）—— 下层通知上层的默认方式

下层只持有一个函数指针，不 `#include` 上层头文件。典型例子是按键驱动：

```c
// driver/drv_button.h —— 下层定义类型 + 注册接口
typedef void (*drv_button_pressed_cb_t)(void *ctx);
ef_err_t drv_button_register_callback(drv_button_pressed_cb_t cb, void *ctx);
```

`app_main.c` 里注册自己的处理函数，`drv_button.c` 完全不知道 `app_main.c` 的存在：

```c
static void on_button_isr(void *ctx) {
    ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, NULL, 0);
}
drv_button_register_callback(on_button_isr, NULL);
```

注意这里回调里做的事：**把"硬件事件"转换成"发布一个业务事件"**，这是回调和发布订阅接力使用的
典型模式——中断上下文只做最少的事，剩下的交给发布订阅去解耦具体谁来处理。

### 5.2 消息队列 —— 中断/生产者与任务/消费者解耦

`drv_uart_console` 是标准示范：HAL 层的 UART 接收中断回调只做一件事——把收到的字节丢进内部的
`osal_queue_t`；真正的消费者（`app_main.c` 主循环喂给 `ef_cli_feed_byte`）用
`drv_uart_console_read_byte()` 带超时地取数据。生产者和消费者的执行节奏完全不同步，靠队列缓冲。

```c
// driver/drv_uart_console.h
ef_err_t drv_uart_console_read_byte(uint8_t *out_byte, uint32_t timeout_ms);
// timeout_ms 支持 OSAL_NO_WAIT（非阻塞轮询）/ OSAL_WAIT_FOREVER（阻塞等待）
```

### 5.3 发布订阅（Pub/Sub）—— 一对多，发布者不关心谁在听

`service/event_bus` 提供精确字符串 topic 匹配的发布订阅。区别于回调的关键点：**发布者不持有、
也不关心具体订阅者是谁**，可以有 0 个、1 个或 N 个订阅者。

```c
// app_led_blink.c 订阅按键事件，和"谁发布了这个事件"完全解耦
ef_event_bus_subscribe(APP_EVENT_BUTTON_PRESSED, app_led_blink_on_button_event, NULL);
```

**选择哪种机制的判断标准**：
- 下层需要"通知"上层一件具体的事，且上层身份在编译期就确定（一对一）→ **回调**
- 生产/消费节奏不同步，需要缓冲 → **消息队列**
- 一个事件可能有多个互不相关的响应者（记日志的同时还要报警）→ **发布订阅**

---

## 6. OSAL：如何做到"业务代码不改一行切换 RTOS"

`osal/include/osal.h` 是业务代码唯一允许 `#include` 的 OS 接口，**禁止在 Driver/Service/App 里
直接出现 `xSemaphoreGive()`、`__disable_irq()` 这类具体后端 API**。三套后端二选一由 CMake 变量
`EF_OSAL_BACKEND=bare|freertos` 在编译期决定（`host` 后端固定用于 PC 单测，不对外暴露为选项）。

| 后端 | mutex 实现 | thread 实现 | 用途 |
|:---|:---|:---|:---|
| `bare` | 关中断 | 协作式轮转（登记进静态表，`osal_scheduler_run_once()` 驱动） | 无 RTOS 的简单场景 |
| `freertos` | `xSemaphoreCreateMutex` | `xTaskCreate` | 需要真正多任务并发 |
| `host` | pthread mutex | pthread | PC 单元测试 |

**裸机后端的刻意限制**（不是缺陷）：没有真正的抢占式调度器和独立堆栈，`osal_thread_create`
只是把入口函数登记进一张静态表，`osal_scheduler_run_once()` 依次调用每个已注册入口一次。
如果只创建 1 个"线程"（本脚手架 App 层示例的用法）且它内部是 `while(1) { work(); osal_delay_ms(...); }`，
效果等价于该函数自己接管主循环——完全正确。**如果创建 2 个以上这种"内部 while(1)"的线程，
第一个会永远占住 CPU**，因为没有真正的上下文切换。真正需要多任务并发时切到
`EF_OSAL_BACKEND=freertos`，业务代码不用改。

---

## 7. 测试策略：为什么 App 层业务代码能在 PC 上跑单测

`test/mocks/drv_led_mock.c` 提供和真实 `driver/drv_led.c` **完全一样的对外函数签名**
（`drv_led_init/on/off/toggle`），`app/app_led_blink.c` 只 `#include "drv_led.h"`（纯声明，
不含任何芯片专属类型），完全不知道自己链接的是真驱动还是这个 mock。这就是"面向接口编程"
带来的可测试性收益——`test/CMakeLists.txt` 里 `test_app_led_blink` 这个目标链接的是
`drv_led_mock`，而不是真实的 `driver` 库。

各层测试策略：

| 层 | Host 单测怎么测 |
|:---|:---|
| `lib/` | 直接编译测试，零依赖 |
| `driver/device`（框架核心） | 直接编译测试，用假的 `ef_device_ops_t` 断言注册/查找/分发逻辑 |
| `driver/drv_*.c`（真实驱动） | **不测**，依赖真实寄存器，只能在真机上跑（或者未来接硬件仿真） |
| `service/*` | 直接编译测试；`ef_log` 用假 sink 捕获输出断言，不需要真 UART |
| `app/*` | 编译测试 + 硬件 mock（如 `drv_led_mock`），验证业务逻辑，不碰任何真实外设 |

新增一个驱动，如果想让上面依赖它的业务逻辑可测，就照 `drv_led_mock` 的样子在 `test/mocks/`
下补一份同签名的 mock。

---

## 8. 命名与错误处理约定

- 函数/类型前缀 `ef_`，宏前缀 `EF_`；驱动模块用 `drv_` 前缀，不叠加 `ef_`（如 `drv_led_init`
  而不是 `ef_drv_led_init`）。
- 统一错误码 `ef_err_t`（`lib/ef_common/include/ef_common.h`）：`EF_OK = 0`，其余全部是负值
  （`EF_ERR_PARAM`、`EF_ERR_TIMEOUT`、`EF_ERR_BUSY`、`EF_ERR_NOMEM`、`EF_ERR_NOTFOUND`、
  `EF_ERR_NOTSUPPORTED`、`EF_ERR_IO`）。所有对外函数的返回值都应该是这套错误码，除非该函数
  本来就要返回字节数（如 `ef_device_read/write`，此时用负值区分错误 vs 正常返回长度）。
- 所有外部输入做合法性检查用 `EF_ASSERT`（真机上默认死循环，host 单测下应覆盖为 `abort()`
  以便测试框架捕获断言失败）。
- 零全局可写状态暴露：模块内部状态用文件内 `static` 变量封装，只通过函数接口访问；测试专用的
  `xxx_reset()`/`xxx_registry_reset()` 是唯一例外，且要在函数注释里写明"仅用于单元测试"。

---

## 9. 已知限制与扩展点

- **裸机调度器单任务限制**：见第 6 节。
- **`ef_log_write` 非重入**：内部用 `static char line[EF_LOG_LINE_MAX]` 格式化，多任务并发打日志
  需要调用方在外面加 `osal_mutex`，本组件不内置锁（避免给裸机单任务场景强加锁开销）。
- **`ef_config` 是纯 RAM 实现**：`ef_config_load`/`ef_config_save` 当前是空实现（总是成功），
  是留给 Flash 持久化（比如接 FlashDB 思路做磨损均衡）的扩展点，接入后 `get`/`set` 调用方零改动。
- **`HAL_GetTick()` 在 `EF_OSAL_BACKEND=freertos` 下的行为**：FreeRTOS 把 `SysTick_Handler` 重映射为
  `xPortSysTickHandler`，ST HAL 的 tick 计数依赖需要单独确认是否仍准确，当前未做特殊处理，
  是接入更多 ST HAL 高层 API（如某些依赖 `HAL_Delay`/超时的外设驱动）前需要验证的点。

发现新的边界情况或者要新增一层通信机制，先更新这份文档再写代码。
