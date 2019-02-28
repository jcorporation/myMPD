#!/bin/sh
g++ -shared -fPIC -o coverextract.so coverextract.cpp

gcc -Wall test_coverextract.c -o test_coverextract coverextract.so -ldl
