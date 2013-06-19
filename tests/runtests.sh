#!/bin/bash

PATH="$PATH:$PWD/../build"

for testfile in *.test
do
  testname=${testfile%*.test}

  echo -en "\033[1m\033[37m[\033[34m      \033[37m]\033[0m $testname"
  bash $testfile > /dev/null 2>&1
  if [ $? -ne 0 ]
  then
    echo -e "\r\033[1m\033[37m[\033[31mFAILED\033[37m]\033[0m $testname"
    echo -e "\033[1m\033[37m[\033[34m RERUN\033[37m]\033[0m $testname"
    bash $testfile
    echo -e "\033[1m\033[37m[\033[34mENDRUN\033[37m]\033[0m $testname"
  else
    echo -e "\r\033[1m\033[37m[\033[32m  OK  \033[37m]\033[0m $testname"
  fi
done
