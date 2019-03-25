#!/bin/sh

if [ -x /usr/bin/cppcheck ]
then
  echo "Running cppcheck"
  cppcheck --enable=warning --inconclusive --force --inline-suppr src/*.c src/*.h
  cppcheck --enable=warning --inconclusive --force --inline-suppr src/plugins/*.c src/plugins/*.h src/plugins/*.cpp
fi
