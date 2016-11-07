#!/bin/sh

idx=0
while true
do
    host="10.11.144.68:48080"

    str=`date +%H%M%S.%N`;
    mesg="{\"EXTID\":4, \"EXTEND\":{\"NAME\":\"LETV-BROADCAST-$idx========================================================\", \"TIME\":\"$str\"}}"

    #curl "http://$host/engine/usrmgr/push?opt=broadcast" -d "$mesg"
    #curl "http://$host/engine/usrmgr/push?opt=broadcast" -d "$mesg"
    curl "http://$host/engine/usrmgr/push?opt=multicast&dimension=appid-and-rid&appid=58&type=2&rid=1234" -d "$mesg"
    #curl "http://$host/engine/usrmgr/push?opt=multicast-by-packagename&packagename=com.letv.iphone.enterprise.client" -d "$mesg"
    #curl 'http://$host/engine/usrmgr/push?opt=multicast-by-packagename&packagename=com.letv.push" -d "$mesg"
    #if [ $idx -eq 100 ]; then
    #    break;
    #fi
    idx=`expr $idx + 1`
    echo $mesg
    sleep 0.1
done
