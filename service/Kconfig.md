# Service 层组件裁剪说明

本脚手架没有引入完整的 Kconfig/menuconfig 工具链（对一个教学向脚手架
来说过重），改用最直接的方式做到"可裁剪"：**每个组件是独立的 CMake
target，互相之间不直接调用（只通过函数指针/事件解耦），不需要某个
组件时，直接在根 `CMakeLists.txt` 里删掉对应的 `add_subdirectory()`
和 `target_link_libraries()` 即可，不会牵连其他组件。**

| 组件 | target 名 | 依赖 | 裁剪影响 |
|---|---|---|---|
| log | `ef_log` | `ef_common` | 去掉后 `EF_LOG_*` 宏调用需要一起删除（或改成空宏） |
| event_bus | `ef_event_bus` | `ef_common` | 去掉后基于 topic 的发布订阅失效，改回直接函数调用/回调 |
| cli | `ef_cli` | `ef_common` | 去掉后失去命令行调试能力，不影响其他组件运行 |
| config | `ef_config` | `ef_common` | 去掉后需要把参数直接写成编译期常量 |

如果后续项目规模变大、组件数量显著增加（10+），再引入真正的
Kconfig/KConfiglib 是合理的演进方向，届时可以参考 RT-Thread 的
`menuconfig` 集成方式，把这张表格自动化。
