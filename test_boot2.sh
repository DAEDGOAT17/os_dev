#!/bin/bash
qemu-system-x86_64 -m 2G -boot d -cdrom jarvis.iso -drive file=disk.img,format=raw -nographic < /dev/null > serial2.log 2>&1 &
PID=$!
sleep 15
kill -9 $PID
cat serial2.log
