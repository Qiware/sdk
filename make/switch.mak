###############################################################################
## Copyright(C) 2015-2025 Letv technology Co., Ltd
##
## 功    能: 宏开关模块
##        通过此模块中各值的设置，可以控制项目源码的编译.
## 注意事项: 
##        请勿随意修改此文件
## 作    者: # Qifeng.zou # 2015.11.05 #
###############################################################################

CONFIG_DEFAULT_SUPPORT = __ON__		# 默认功能开关
CONFIG_DEBUG_SUPPORT = __ON__		# 调试开关
CONFIG_MEMALIGN_SUPPORT = POSIX_MEMALIGN	# MEMALIGN # 内存对齐方式
CONFIG_MEMLEAK_CHECK = __OFF__		# 内存泄露测试
CONFIG_CONN_POOL_SUPPORT = __ON__	# 开启连接池
CONFIG_JEMALLOC = __OFF__ # JE MALLOC内存池开关
