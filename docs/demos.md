# 学习向 Demo 清单

这份清单是给"拿着 embed-forge 学嵌入式分层架构"用的：每个 demo 对应一个具体的层/机制，
从不需要硬件的到需要真机的，从改现有模块到新增模块，难度递增。**改之前先看
[docs/architecture.md](architecture.md) 的分层判断表，改完对着
[CONTRIBUTING.md](../CONTRIBUTING.md) 的检查清单过一遍。**

改动通用套路（新增驱动/Service 别手写骨架）：

```bash
tools/new_driver.sh <name>    # 生成 driver/drv_<name>.h/.c
tools/new_service.sh <name>   # 生成 service/<name>/...
```

---

## 难度分级

| 级别 | 需要什么 | 对应 demo |
|:---|:---|:---|
| L0 · 不需要硬件 | 只需要本机 gcc + CMake | Demo 1 |
| L1 · 改现有驱动 | Nucleo-F411RE 真机 | Demo 2、3 |
| L2 · 改 Service 层 | 真机（或 host，看 demo） | Demo 4、7 |
| L3 · 新增完整驱动 | 真机 + 一个外部模块（传感器/Flash） | Demo 5、8 |
| L4 · OSAL 后端切换 | 真机 | Demo 6 |

---

## Demo 1（L0）：不装板子，在 PC 上跑一个交互式 CLI

**学什么**：Service 层（`ef_cli`/`ef_event_bus`/`ef_log`）+ App 层业务逻辑本身跟硬件无关，
可以完全在 PC 上跑起来，这正是分层架构"可测试性"的直接体现。

**怎么改**：

1. 新建 `test/host/demo_cli_repl.c`（不是 Unity 测试，是一个交互式小程序）：读 stdin 一个
   字节就 `ef_cli_feed_byte()`，输出 sink 接到 `printf`。
2. 复用 `app_led_blink.c` + `test/mocks/drv_led_mock.c`：注册一个 CLI 命令 `press`，调用
   `ef_event_bus_publish(APP_EVENT_BUTTON_PRESSED, ...)`，再注册一个 `status` 命令打印
   `app_led_blink_get_led_state()`/`get_press_count()`。
3. `test/CMakeLists.txt` 加一个 `add_executable(demo_cli_repl ...)`（不用 `ef_add_host_test`，
   不注册进 ctest，它是跑给人看的，不是自动化测试）。
4. `cmake --build --preset host --target demo_cli_repl && ./build-host/test/demo_cli_repl.exe`，
   敲 `help`、`press`、`status`，看事件总线怎么把"按键"和"业务状态"串起来。

**验证**：敲命令能看到 `press_count` 递增，`help` 能列出所有注册的命令。

---

## Demo 2（L1）：LED 从"开关"改成"呼吸灯"（PWM）

**学什么**：HAL 层怎么加一个新外设能力（PWM），Driver 层怎么在不改对外接口签名的前提下
换掉内部实现。

**怎么改**：

1. `hal/include/hal_pwm.h`：声明 `hal_pwm_init(config)`、`hal_pwm_set_duty(channel, duty_permille)`。
2. `hal/stm32f4xx/hal_pwm_stm32f4.c`：包一层 `HAL_TIM_PWM_*`。
3. `board/nucleo_f411re/board_config.h`：加 LED 对应定时器通道的引脚/AF 定义
   （Nucleo-F411RE 板载 LED 在 PA5，看能不能配到 `TIM2_CH1`；若不行换一个能出 PWM 的引脚接外部 LED）。
4. `driver/drv_led.c`：`drv_led_init()` 内部改成调 `hal_pwm_init`；新增
   `drv_led_set_brightness(uint8_t percent)`，`drv_led_on/off/toggle` 保留（映射成 100%/0%），
   **对外 `drv_led.h` 的既有函数签名不变**——这是"驱动层屏蔽实现细节"的关键验证点。
5. `app_led_blink.c`：把 `drv_led_toggle()` 换成按固定步长递增/递减调用 `drv_led_set_brightness()`
   做呼吸效果。

**验证**：烧录后 LED 应该是渐亮渐暗而不是硬切换；`test_app_led_blink` 如果直接用了
`drv_led_toggle`，要同步在 `test/mocks/drv_led_mock.c` 里补 `drv_led_set_brightness` 的 mock。

---

## Demo 3（L1）：按键区分"短按/长按"

**学什么**：怎么在 Driver 层内部维护"状态"（不是无状态的转发），以及发布订阅怎么承载更丰富的事件语义。

**怎么改**：

1. `driver/drv_button.h`：把 `drv_button_pressed_cb_t` 改成带一个 `press_kind`（`SHORT`/`LONG`
   枚举）参数，或者新增一个 `drv_button_register_long_press_callback`。
2. `driver/drv_button.c`：中断回调里记录按下时刻（`osal_tick_ms()`），松开时刻减去按下时刻，
   超过阈值（比如 800ms）判定为长按，分别触发对应回调。
3. `app_main.c`：注册两个 event_bus topic（`btn_short_press` / `btn_long_press`），短按切换 LED，
   长按触发 `EF_LOG_W` 打一条警告日志模拟"进入配置模式"。

**验证**：`test/host/test_app_led_blink.c` 或新写一个测试，直接调用回调函数模拟"短按"和"长按"
两种输入，断言触发的 topic 不一样。

---

## Demo 4（L2，纯 host 也能验证逻辑）：CLI 加一个 `info` / `config` 命令

**学什么**：怎么新增 CLI 命令、`ef_config` 的读写调用方式。

**怎么改**：

1. `app_main.c`（或者单独拆一个 `app_cli_commands.c`）新增：
   - `info`：打印 `EF_OSAL_BACKEND`（编译宏）、`osal_tick_ms()`、固件版本号（自己定义一个
     `#define EF_FW_VERSION "0.1.0"` 放 `ef_common.h` 或新建 `version.h`）。
   - `config get/set <key> <value>`：包一层 `ef_config_get_i32`/`set_i32`，让你在真机上通过
     串口终端直接改配置，不用重新烧录。
2. host 上可以直接在 Demo 1 的 REPL 里验证这两个命令，不需要真机。

**验证**：`config set baud 9600` 后 `config get baud` 能读回 `9600`；重启（真机）后应该读不到
（因为 `ef_config` 目前是 RAM 版——这一步能帮你直观感受到 Demo 7 要解决的问题）。

---

## Demo 5（L3）：接一个真实 I2C 传感器（比如 AHT20 温湿度）

**学什么**：完整走一遍"新增 HAL 能力 → 新增驱动 → 注册 `ef_device` → CLI 查询"的标准流程。

**怎么改**：

1. `hal/include/hal_i2c.h` + `hal/stm32f4xx/hal_i2c_stm32f4.c`：包一层 `HAL_I2C_Master_Transmit/Receive`。
2. `tools/new_driver.sh aht20` 生成骨架，改成：
   - `drv_aht20_init()` 发触发测量命令；
   - `drv_aht20_read(float *temp_c, float *humidity_pct)` 读 6 字节数据并解析；
   - 注册成 `ef_device`（`ef_device_register(&dev, "aht20", &ops, NULL)`），`ops.read` 走统一接口
     （返回原始字节，parse 逻辑放调用方或者驱动内部包一层）。
3. CLI 加 `sensor` 命令：`ef_device_find("aht20")` + `ef_device_read` 打印温湿度。

**验证**：`test/mocks/` 补一份 `drv_aht20_mock.c`（固定返回一组假数据），验证 CLI 命令解析、
打印格式在 host 上是对的；真实读数准不准要接真硬件量。

---

## Demo 6（L4）：从裸机切到 FreeRTOS，感受 OSAL 存在的意义

**学什么**：`docs/architecture.md` 第 6 节讲的"裸机后端单任务限制"到底长什么样，以及切 RTOS
后业务代码为什么不用改。

**怎么改**：

1. 先在 `EF_OSAL_BACKEND=bare` 下，往 `app_main.c` 加第二个 `osal_thread_create`（比如一个
   "看门狗心跳"任务，内部也是 `while(1) { ...; osal_delay_ms(...); }`）——**验证性地看到第二个
   任务永远不会被调度到**，对应 `docs/architecture.md` 第 6 节写的限制。
2. `make target-freertos` 重新编译（`EF_OSAL_BACKEND=freertos`），**app 层代码一行不改**，
   两个任务应该都能正常轮转执行。
3. 给两个任务之间加一个共享变量（比如心跳计数），用 `osal_mutex_lock/unlock` 保护，体会为什么
   多任务并发下裸机的"关中断当 mutex"策略在真正抢占式调度下也依然成立、但你需要真的去用它。

**验证**：裸机版本第二个任务的日志应该完全不出现；FreeRTOS 版本两个任务的日志应该交替出现。

---

## Demo 7（L2/L3）：`ef_config` 从 RAM 升级成真正写 Flash

**学什么**：`ef_config_load`/`ef_config_save` 这两个"预留扩展点"具体怎么接进去，且上层
`get`/`set` 调用方零改动。

**怎么改**：

1. `hal/include/hal_flash.h` + 实现：封装 `HAL_FLASH_Program`/`HAL_FLASH_Unlock` 等，只暴露
   "擦一个扇区"/"写一段数据"/"读一段数据"这种语义化接口（别把 `FLASH_TypeDef` 泄漏出去）。
2. `service/config/ef_config.c`：`ef_config_save()` 把 `s_entries` 数组整个序列化写进指定 Flash
   扇区；`ef_config_load()` 反过来读回来（注意 STM32F4 扇区擦除是必须先擦后写，且擦除以扇区为
   单位，别直接在运行时随便擦，规划好用最后一个扇区存配置）。
3. 触发时机：`ef_cli` 加一个 `config save` 命令手动触发，不要在每次 `set` 时自动写（Flash 写
   次数有限，频繁写会加速磨损，这也是为什么 `docs/architecture.md` 提到"接 FlashDB 磨损均衡"
   的原因——这一版先不做磨损均衡，只做最基本的能持久化）。

**验证**：`config set x 1` → `config save` → 断电重启 → `config get x` 应该还是 `1`。

---

## Demo 8（L3，综合）：新增一个 Service 组件——周期性状态上报

**学什么**：`EF_INIT_EXPORT` + 事件总线订阅多个 topic 汇总状态，练一遍"新增 Service"的完整流程。

**怎么改**：

1. `tools/new_service.sh status_reporter` 生成骨架。
2. 订阅 `btn_pressed`（或 Demo 3 的 `btn_short_press`/`btn_long_press`）累计计数，同时持有最近一次
   LED 状态（可以直接读 `app_led_blink_get_led_state()`，或者更解耦地让 App 层也 publish 一个
   `led_state_changed` 事件，Service 层不反向依赖 App 层的头文件）。
3. 内部起一个"每 N 秒"的逻辑（用 `osal_tick_ms()` 在 App 层主循环里轮询调用
   `ef_status_reporter_tick()`，不要在 Service 层自己开线程，保持"Service 层不管调度"的边界），
   拼一段 JSON 风格字符串，通过 `drv_uart_console_write` 打印出去。

**验证**：串口终端应该每隔固定时间看到一行状态 JSON，按键次数和 LED 状态跟实际操作一致。

---

## 做完一个 demo 之后

- 补 host 单测（哪怕只是验证纯逻辑部分，硬件相关的部分测不了就明确跳过，别硬凑假测试）。
- 检查有没有违反分层规则（`docs/architecture.md` 第 1 节的自查表）。
- 如果新增了通信机制或者踩了新坑，回头更新 `docs/architecture.md`——这份清单存在的意义就是
  "学一个概念，同时把项目文档也喂饱"，不是写完就扔。
