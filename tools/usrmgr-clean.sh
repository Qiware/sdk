#!/bin/sh

# 线上环境
#IP=10.100.54.31
#PORT=7379
# 联合测试
IP=10.180.91.50
PORT=14321
PASSWD=BuD5XTpmCr27ewEP
# 开发测试
#IP=10.58.89.236
#PORT=8380

redis-cli -h $IP -p $PORT -a $PASSWD KEYS usrmgr:* > keys.txt

keys=""
idx=1
while read line
do
    mod=`expr $idx % 100`
    if [ $mod -eq 0 ]; then
        echo "redis-cli -h $IP -p $PORT -a $PASSWD DEL $keys"
        redis-cli -h $IP -p $PORT -a $PASSWD DEL $keys
        keys=""
    fi
    keys="$keys $line"
    idx=`expr $idx + 1`
done < keys.txt

redis-cli -h $IP -p $PORT -a $PASSWD DEL $keys
