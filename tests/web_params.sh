#!/bin/bash
function assertEquals()
{
    msg=$1; shift
    expected=$1; shift
    actual=$1; shift
    if [ "$expected" != "$actual" ]; then
        echo "$msg EXPECTED=$expected ACTUAL=$actual"
        exit 2
    else
        echo "OK"
    fi
}

assertEquals "simple get :" "GET	/echo" "`curl -s 127.0.0.1:1999/echo`" 
assertEquals "simple post:" "a	b" "`curl -s 127.0.0.1:1999/echo -X POST --data 'a=b&c=d' | grep ^a`" 

