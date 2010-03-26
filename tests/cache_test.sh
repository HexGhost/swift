#!/bin/bash

TIMETRANS=3 #time for transmission. Increase in case of error in transmitting test.
TIMECACHE=1 #time for work with cache.
PORT=2000
#caches for blank files of 1,2,4 megabyte.
HASH1M=3ca46f6199467a8b029f188401021202086e286c
HASH2M=4bfe3bbbc17512ae798bd5ddeae2b4b2d01bf05e
HASH4M=9ded682e938a360c9e62d34de5c9fad45143450f

#kill swift instances
function killswift {
  killall -q -9 swift
}

#kill swift and remove test files and cache.
function bye {
  killswift
  rm -r cache_test_tmp*
}

killswift

echo -n creating files
if (dd if=/dev/zero of=cache_test_tmp_1M bs=1M count=1 2> /dev/null && 
    dd if=/dev/zero of=cache_test_tmp_2M bs=2M count=1 2> /dev/null &&
    dd if=/dev/zero of=cache_test_tmp_4M bs=4M count=1 2> /dev/null);
then
  echo -e '\e[33m\e[5m OK\e[0m'
else
  echo -e '\e[33m\e[5m FAIL\e[0m'
  bye
  exit 1
fi

function transmit {
  echo -n transmitting $1 file
  ../swift -f cache_test_tmp_$1 -l $PORT > /dev/null 2> /dev/null &
  ../swift --cache-dir cache_test_tmp --cache-size 6500000 -h $2 -t localhost:$PORT &
  #PORT=$(($PORT+1))
  sleep $TIMETRANS
  if [ "`ps | grep -c swift`" == "1" ];  
  then
    echo -e '\e[33m\e[5m OK\e[0m'
  else
    echo -e '\e[33m\e[5m FAIL\e[0m'
    bye
    exit 1
  fi
  killswift
}

#transmit files from seeder to leecher's cache
transmit 1M $HASH1M
transmit 2M $HASH2M
transmit 4M $HASH4M

function check_in_cache {
  echo -n check_in_cache $1 absence=$2 file
  ../swift --cache-dir cache_test_tmp -h $1 -t localhost:$PORT &
  sleep $TIMECACHE
  if [ "`ps | grep -c swift`" == $2 ];  
  then
    echo -e '\e[33m\e[5m OK\e[0m'
  else
    echo -e '\e[33m\e[5m FAIL\e[0m'
    bye
    exit 1
  fi
  killswift
}

#we use cache size 6500000, so oldest files should not be in cache
check_in_cache $HASH1M 1 #absence
check_in_cache $HASH2M 0 #presence
check_in_cache $HASH4M 0 #presence

../swift --cache-dir cache_test_tmp --cache-size 4500000 -h $HASH4M -t localhost:$PORT &
sleep $TIMECACHE
killswift

#space (4500000) only for 4m file
check_in_cache $HASH1M 1 #absence
check_in_cache $HASH2M 1 #absence
check_in_cache $HASH4M 0 #presence

../swift --cache-dir cache_test_tmp --cache-size 100 -h $HASH4M -t localhost:$PORT &
sleep $TIMECACHE
killswift

#no space - remove all files
check_in_cache $HASH1M 1 #absence
check_in_cache $HASH2M 1 #absence
check_in_cache $HASH4M 1 #absence

bye