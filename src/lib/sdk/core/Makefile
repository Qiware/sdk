###############################################################################
## Copyright(C) 2015-2025 Letv technology Co., Ltd
##
## 文件名: Makefile
## 版本号: 1.0
## 描  述: 核心模块的编译
## 作  者: # Qifeng.zou # 2015.11.03 14:58:54 #
###############################################################################
include $(PROJ)/make/build.mak

INCLUDE = -I. \
		  -I$(PROJ)/src/incl
INCLUDE += $(G_INCLUDE)
LIBS_PATH = -L$(PROJ)/lib
LIBS = -lpthread -lev

SRC_LIST = str.c \
		   log.c \
		   ring.c \
		   slot.c \
		   util.c \
		   redo.c \
		   list.c \
		   list2.c \
		   lock.c \
		   iovec.c \
		   queue.c \
		   thread_pool.c \
		   avl_tree.c \
		   rb_tree.c \
		   xml_comm.c \
		   xml_tree.c \
		   xml_print.c \
		   hash_tab.c \
		   zmalloc.c

OBJS = $(subst .c,.o, $(SRC_LIST))
HEADS = $(call func_get_dep_head_list, $(SRC_LIST))

TARGET = libcore.a

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJS)
	@$(AR) $(AFLAGS) -s $@ $(OBJS)
	@echo "AR $@"
	@mv $@ $(PROJ_LIB)

$(OBJS): %.o : %.c $(HEADS)
	@$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)
	@echo "CC $(PWD)/$<"

clean:
	@rm -fr *.o $(PROJ_LIB)/$(TARGET)
	@echo "rm -fr *.o $(PROJ_LIB)/$(TARGET)"
