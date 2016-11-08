###############################################################################
## Copyright(C) 2013-2014 Qiware technology Co., Ltd
##
## 文件名: Makefile
## 版本号: 1.0
## 描  述: 实时消息队列(RTMQ)模块的编译
##         RTMQ: 实时消息队列
##         RTSD: 实时发送精灵
##         RTRD: 实时接收精灵
## 作  者: # Qifeng.zou # 2015.06.05 23:14:02 #
###############################################################################
include $(PROJ)/make/build.mak

INCLUDE = -I. \
		  -I$(PROJ)/src/incl \
		  -I$(PROJ)/src/incl/sdk \
		  -I$(PROJ)/src/incl/mesg
INCLUDE += $(GLOBAL_INCLUDE)
LIBS_PATH = -L$(PROJ)/lib
# 注: 静态库请放在动态库之前
STATIC_LINK_LIB_LIST = libcjson.a
STATIC_LINK_LIB_LIST += $(G_STATIC_LIBS)
LIBS = $(call func_find_static_link_lib,$(STATIC_LINK_LIB_PATH),$(STATIC_LINK_LIB_LIST))
LIBS += -lpthread -lm -lcurl -lprotobuf-c
LIBS += $(SHARED_LIB)

SRC_LIST = ./sdk.c \
		   ./sdk_ssvr.c \
		   ./sdk_worker.c \
		   ./core/str.c \
		   ./core/log.c \
		   ./core/logsvr.c \
		   ./core/ring.c \
		   ./core/slot.c \
		   ./core/redo.c \
		   ./core/list.c \
		   ./core/list2.c \
		   ./core/lock.c \
		   ./core/iovec.c \
		   ./core/queue.c \
		   ./core/sck_tcp.c \
		   ./core/sck_udp.c \
		   ./core/sck_unix.c \
		   ./core/thread_pool.c \
		   ./core/avl_tree.c \
		   ./core/rb_tree.c \
		   ./core/xml_comm.c \
		   ./core/xml_tree.c \
		   ./core/xml_print.c \
		   ./core/hash_tab.c \
		   ./mesg/mesg.pb-c.c

OBJS = $(subst .c,.o, $(SRC_LIST))
HEADS = $(call func_get_dep_head_list, $(SRC_LIST))

TARGET = libsdk.so
TARGET_A = libsdk.a

.PHONY: all clean

all: $(TARGET) $(TARGET_A)
$(TARGET): $(OBJS)
	@$(CC) $(LFLAGS) -o $@ $(OBJS) $(INCLUDE) $(LIBS_PATH) $(LIBS)
	@echo "CC $@"
	@mv $@ $(PROJ_LIB)
	@echo "$@ is OK!"

$(TARGET_A): $(OBJS)
	@$(AR) $(AFLAGS) -s $@ $(OBJS)
	@echo "AR $@"
	@mv $@ $(PROJ_LIB)
	@echo "$@ is OK!"

$(OBJS): %.o : %.c $(HEADS)
	@$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)
	@echo "CC $(PWD)/$<"

clean:
	@rm -fr server/*.o proxy/*.o $(PROJ_LIB)/$(TARGET) $(PROJ_LIB)/$(TARGET_A)
	@echo "rm -fr server/*.o proxy/*.o $(PROJ_LIB)/$(TARGET) $(PROJ_LIB)/$(TARGET_A)"
