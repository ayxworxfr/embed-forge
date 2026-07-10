# 贡献指南

提交代码前请先读 [docs/architecture.md](docs/architecture.md)——那里是判断"这段代码该放哪一层"的
硬性标准，本文档只讲具体操作流程和编码规范。

## 开始之前

```bash
git clone --recurse-submodules <repo-url>
# 已经克隆过忘了带 submodule？
git submodule update --init --recursive
```

## 分层规则（硬性，Review 时第一条检查项）

- **上层可以 `#include` 下层头文件，下层永远不 `#include` 上层头文件。** 比如
  `driver/drv_led.c` 不能 `#include "app_led_blink.h"`；`hal/` 不能出现 `service/` 或 `app/` 的任何符号。
- Driver 对上暴露的是"设备语义"（`drv_led_toggle()`），不是"硬件语义"（`hal_gpio_write()`）。
  如果你在写 driver 时发现要在 `.h` 里暴露 `GPIO_PIN_x` 之类的芯片专属类型，说明这段代码写错层了。
- Service 组件之间不直接互相调用，通过 `service/event_bus` 解耦（参考 `docs/architecture.md` 第 5 节
  三种通信机制怎么选）。
- App 层不允许出现任何 HAL/寄存器/OSAL 原语（`__disable_irq`、`xSemaphoreGive` 等）——如果
  App 层代码里出现这些，说明该加一层抽象而不是绕过它。

不确定该放哪层，看 [docs/architecture.md 第 1 节](docs/architecture.md#1-分层与依赖方向) 的自查表。

## 新增一个驱动

1. `driver/drv_xxx.h` / `driver/drv_xxx.c`，接口暴露"做什么"不暴露"怎么做"（参考 `drv_led.h`）。
2. 需要被应用层按名字查找的，注册成 `ef_device`：

   ```c
   static ef_device_t s_dev;
   static const ef_device_ops_t s_ops = { .write = drv_xxx_write, /* 其余留 NULL */ };
   ef_device_register(&s_dev, "xxx0", &s_ops, NULL);
   ```

3. 文件末尾加自动注册（不需要手动改 `app_main.c`）：

   ```c
   EF_INIT_EXPORT(drv_xxx_init, EF_INIT_LEVEL_DRIVER);
   ```

4. 加进 `driver/CMakeLists.txt` 的源文件列表。真实驱动依赖 `hal`/`board`，**只在交叉编译时构建**，
   不需要（也不应该）让它在 host 单测的 CMake 分支里可见。
5. 如果上层业务逻辑（App/Service）需要依赖这个驱动做单测，在 `test/mocks/` 补一份同函数签名的
   mock（参考 `test/mocks/drv_led_mock.c`），host 侧链接 mock 而不是真实实现。

## 新增一个 Service 组件

1. `service/xxx/include/ef_xxx.h` + `service/xxx/ef_xxx.c` + `service/xxx/CMakeLists.txt`
   （参考 `service/log/`）。
2. Service 组件必须能在 host 上独立编译测试——不允许出现 `hal_*`、`board_config.h`、
   芯片专属类型。唯一允许的外部依赖是 `ef_common`、`osal.h`（如果需要 mutex/queue）、
   `ef_event_bus`（如果需要发布订阅）。
3. `ef_xxx_init()` 末尾加 `EF_INIT_EXPORT(ef_xxx_init, EF_INIT_LEVEL_SERVICE);`。
4. 加一个 `xxx_reset()`（仅测试用，清空内部静态状态，避免测试用例互相污染），在函数注释里写明
   "仅用于单元测试"。
5. 在 `service/CMakeLists.txt` 加 `add_subdirectory(xxx)`，在根 `CMakeLists.txt` 的两个分支
   （host / `CMAKE_CROSSCOMPILING`）都会自动带上，不用改根文件。
6. 在 `test/CMakeLists.txt` 用 `ef_add_host_test()` 注册对应的单测可执行文件。

## 编码规范

- 命名：函数/类型 `ef_` 前缀，宏 `EF_` 前缀；驱动模块用 `drv_` 前缀（不叠加 `ef_`）。
- 错误处理：对外函数统一返回 `ef_err_t`（`EF_OK` = 0，其余负值），除非函数本来就要返回长度
  （如 `ef_device_read/write`，用负值区分错误）。
- 所有外部输入做合法性检查（`NULL` 指针、长度越界等），用 `EF_ASSERT` 或返回 `EF_ERR_PARAM`，
  不要假设调用方一定传对。
- 零动态内存分配：不出现运行期 `malloc`/`free`，需要缓冲区/存储区的地方用静态数组或调用方
  传入的存储（参考 `ef_ringbuffer_t`、`ef_device_t`）。
- 零裸暴露的全局可写状态：模块状态封装成文件内 `static` 变量，只通过函数接口访问。
- 中文注释、英文代码/日志。注释解释"为什么这么设计/边界在哪"，不要写"自明代码"的翻译式注释。
- 源文件保存为 UTF-8（无 BOM）。
- **写注释时避免在 `/* ... */` 块注释里出现字面的 `*/` 两字符序列**（比如举例路径通配符
  `board/*/*.ld`），这会让注释提前闭合，后面的内容被当成代码解析，报一堆看起来像编码问题、
  实际是语法错误的 `stray` token 报错。举例路径请写成 `board/nucleo_f411re/xxx.ld` 之类
  的具体路径，不要用 `*` 通配符占位。

## 跑测试

```bash
make host-test
```

已实测：clean 状态下跑 `make host-test`，7 个测试目标全部编译通过并 Passed（ctest 100%）。

新增/修改代码后，确保：
- 涉及的模块有对应的 host 单测（`test/host/test_xxx.c`），且新增测试跑得过。
- 没有破坏其他模块原有的测试。

真机固件的交叉编译验证见 [README.md](README.md#2-交叉编译真机固件需要-arm-none-eabi-gcc)。

## 提交前检查清单

- [ ] 新代码放的层对不对？有没有出现"下层 include 上层"？
- [ ] 是否需要新增/更新 host 单测？
- [ ] 是否需要在 `test/mocks/` 补硬件 mock？
- [ ] 错误处理是否统一用 `ef_err_t`？边界输入是否做了校验？
- [ ] 有没有引入运行期 `malloc`/`free`？
- [ ] 是否更新了 `docs/architecture.md`（如果引入了新的通信机制/设计决策）？

## 已知未解决问题（不要假装它们不存在）

- 本机终端代码页是 GBK(936) 不是 UTF-8(65001)，`make help` 等命令输出的中文提示会显示乱码。
  纯显示问题，不影响文件内容和编译结果，运行 `chcp 65001` 切到 UTF-8 代码页可以解决。
