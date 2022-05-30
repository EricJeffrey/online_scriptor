#!/bin/sh
g++ -o testing/$1.out -D TESTING -g -std=c++17 $1.cpp -lpthread