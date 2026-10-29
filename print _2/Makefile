# ============================================================
#  STM32F103C8T6 (IIC 工程) —— 命令行快捷目标
#
#  用法:  make <目标>        例:  make flash
#  查看全部目标:  make help
#
#  说明: 本文件只服务于 make, 与 CMake/Ninja 构建互不影响
#        (CMake 产物在 build/Debug, 由 cmake --build 驱动)
# ============================================================

# /dev/tcp 需要 bash (Ubuntu 默认 sh=dash 不支持)
SHELL := /bin/bash

ELF         ?= build/Debug/IIC.elf
OCD         ?= openocd
OCD_SCRIPTS ?= /usr/share/openocd/scripts
OCD_IFACE   ?= interface/stlink.cfg
OCD_TARGET  ?= target/stm32f1x.cfg
OCD_CFG     := -s $(OCD_SCRIPTS) -f $(OCD_IFACE) -f $(OCD_TARGET)
GDB         ?= gdb-multiarch

.PHONY: all build configure flash debug reset halt erase size hex clean help

all: build

## 配置 + 构建 Debug 固件
build:
	rm -rf build
	cmake --preset Debug
	cmake --build --preset Debug -j$(shell nproc)

## 只运行 CMake 配置 (不编译)
configure:
	cmake --preset Debug

## 烧录 + 校验 + 复位          <<< 最常用
flash:
	@test -f $(ELF) || { echo "未找到 $(ELF), 请先执行 make build"; exit 1; }
	$(OCD) $(OCD_CFG) -c "program $(ELF) verify reset exit"

## 复位并运行
reset:
	$(OCD) $(OCD_CFG) -c "init; reset run; exit"

## 复位并暂停
halt:
	$(OCD) $(OCD_CFG) -c "init; reset halt; exit"

## 全片擦除
erase:
	$(OCD) $(OCD_CFG) -c "init; reset halt; stm32f1x mass_erase 0; exit"

## 启动 openocd + gdb 交互调试 (自动停在 main)
debug:
	@test -f $(ELF) || { echo "未找到 $(ELF), 请先执行 make build"; exit 1; }
	@$(OCD) $(OCD_CFG) & \
	  pid=$$!; \
	  trap 'kill $$pid 2>/dev/null' EXIT INT TERM; \
	  for i in $$(seq 1 50); do \
	    (exec 3<>/dev/tcp/127.0.0.1/3333) 2>/dev/null && { exec 3>&-; break; }; \
	    sleep 0.1; \
	  done; \
	  $(GDB) -q $(ELF) \
	    -ex "target extended-remote :3333" \
	    -ex "monitor reset halt" \
	    -ex "load" \
	    -ex "break main" \
	    -ex "continue"

## 查看 Flash / RAM 占用
size:
	@test -f $(ELF) || { echo "未找到 $(ELF), 请先执行 make build"; exit 1; }
	arm-none-eabi-size $(ELF)

## 生成 .hex 与 .bin
hex:
	@test -f $(ELF) || { echo "未找到 $(ELF), 请先执行 make build"; exit 1; }
	arm-none-eabi-objcopy -O ihex   $(ELF) $(basename $(ELF)).hex
	arm-none-eabi-objcopy -O binary $(ELF) $(basename $(ELF)).bin
	@ls -l $(basename $(ELF)).hex $(basename $(ELF)).bin

## 删除 build 目录
clean:
	rm -rf build

## 显示本帮助
help:
	@echo "可用目标:"
	@grep -E '^## ' $(MAKEFILE_LIST) | sed 's/^## /  make /'
