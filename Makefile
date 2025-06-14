.PHONY: all
all:
	gcc wlink.c -O2 -o bin/wlink -lkernel32 -lshlwapi

