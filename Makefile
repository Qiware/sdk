###############################################################################
## Copyright(C) 2015-2025 Letv technology Co., Ltd
##
## 功    能: 遍历编译目录，并执行指定的操作
##			1. 编译操作
##			2. 删除操作
##			3. 重新编译 
## 注意事项: 
##			1. 当需要增加编译目录时, 请将目录加入变量DIR中, 不用修改该文件其他数据!
## 			2. 如果只想编译指定目录的代码, 则可执行命令:
## 				Make DIR=指定目录 如: Make DIR=src/lib/core
## 作    者: # Qifeng.zou # 2015.11.03 #
###############################################################################
include ./make/func.mak

export VERSION=p.1.3.0 # 系统版本号

# 根目录
export PROJ = ${PWD}
export PROJ_3RD = ${PROJ}/3rd
export PROJ_BIN = ${PROJ}/bin
export PROJ_LIB = ${PROJ}/lib
export PROJ_CONF = ${PROJ}/conf

# 编译目录(注：编译按顺序执行　注意库之间的依赖关系)
LIB_DIR = "src/lib"
DIR += "$(LIB_DIR)/sdk"

DEMO_DIR = "src/demo"
DIR += "$(DEMO_DIR)/sdk"

# 获取系统配置
CPU_CORES = $(call func_cpu_cores)

.PHONY: all clean rebuild help

# 1. 编译操作
all:
	$(call func_mkdir)
	@for ITEM in ${DIR}; \
	do \
		if [ -e $${ITEM}/Makefile ]; then \
			cd $${ITEM}; \
			make 2>&1 || exit; \
			cd ${PROJ}; \
		fi \
	done

# 2. 清除操作
clean:
	@for ITEM in ${DIR}; \
	do \
		if [ -e $${ITEM}/Makefile ]; then \
			cd $${ITEM}; \
			make clean; \
			cd ${PROJ}; \
		fi \
	done

# 3. 重新编译 
rebuild: clean all

# 4. 显示帮助
help:
	@cat make/help.mak
