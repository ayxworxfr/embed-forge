# ============================================================
# embed-forge Makefile
# Usage: make <command>
# 对 CMakePresets 的薄包装，真正的构建逻辑都在 CMakeLists.txt /
# CMakePresets.json 里，这里不重复实现。
# ============================================================

.PHONY: help deps host host-test target-bare target-freertos flash-bare flash-freertos format format-check clean

.DEFAULT_GOAL := help

GREEN  := \033[0;32m
YELLOW := \033[0;33m
CYAN   := \033[0;36m
NC     := \033[0m

##@ 帮助信息

help: ## 显示此帮助信息
	@echo "$(CYAN)embed-forge$(NC) - 嵌入式分层架构脚手架"
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf "$(YELLOW)用法:$(NC)\n  make $(GREEN)<target>$(NC)\n"} /^[a-zA-Z_-]+:.*?##/ { printf "  $(GREEN)%-16s$(NC) %s\n", $$1, $$2 } /^##@/ { printf "\n$(YELLOW)%s$(NC)\n", substr($$0, 5) } ' $(MAKEFILE_LIST)
	@echo ""
	@echo "$(CYAN)快速开始:$(NC)"
	@echo "  1. make deps           # 首次克隆后同步 third_party submodule"
	@echo "  2. make host-test      # 先确认 PC 单测能跑通"
	@echo "  3. make target-bare    # 需要 arm-none-eabi-gcc，编译真机固件"
	@echo ""

##@ 依赖

deps: ## 初始化/同步 git submodule（third_party/）
	@echo "$(GREEN)📦 同步 third_party submodule...$(NC)"
	@git submodule update --init --recursive
	@echo "$(GREEN)✅ submodule 已就绪$(NC)"

##@ PC 单元测试

host: ## 配置 + 编译 PC 单元测试（build-host/）
	@echo "$(GREEN)🔨 配置 + 编译 host 单测...$(NC)"
	@cmake --preset host
	@cmake --build --preset host -j

host-test: host ## 跑 PC 单元测试（自动先 make host）
	@echo "$(GREEN)🧪 运行 host 单测...$(NC)"
	@ctest --preset host

##@ 真机固件（需要 arm-none-eabi-gcc）

target-bare: ## 交叉编译真机固件，OSAL=bare
	@echo "$(GREEN)🔨 交叉编译真机固件（OSAL=bare）...$(NC)"
	@cmake --preset target-bare
	@cmake --build --preset target-bare -j

target-freertos: ## 交叉编译真机固件，OSAL=freertos
	@echo "$(GREEN)🔨 交叉编译真机固件（OSAL=freertos）...$(NC)"
	@cmake --preset target-freertos
	@cmake --build --preset target-freertos -j

flash-bare: target-bare ## 编译并烧录 bare 固件到 Nucleo-F411RE（需要 st-flash/openocd）
	@echo "$(GREEN)⚡ 烧录 bare 固件...$(NC)"
	@bash tools/flash.sh target-bare

flash-freertos: target-freertos ## 编译并烧录 freertos 固件到 Nucleo-F411RE（需要 st-flash/openocd）
	@echo "$(GREEN)⚡ 烧录 freertos 固件...$(NC)"
	@bash tools/flash.sh target-freertos

##@ 代码质量

format: ## 用 clang-format 原地格式化所有 C 源码
	@echo "$(GREEN)✨ 格式化 C 源码...$(NC)"
	@find app board driver hal lib osal service test -name '*.c' -o -name '*.h' | xargs clang-format -i

format-check: ## 只检查格式，不修改文件（CI 用）
	@echo "$(GREEN)🔍 检查代码格式...$(NC)"
	@find app board driver hal lib osal service test -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror

##@ 清理

clean: ## 删除所有 build-* 目录
	@echo "$(YELLOW)🧹 清理构建产物...$(NC)"
	@rm -rf build-host build-target-bare build-target-freertos
	@echo "$(GREEN)✅ 清理完成$(NC)"
