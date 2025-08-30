CC = gcc
CFLAGS = -Wall -O2 -Wno-format-zero-length

all: jpgdtest jpgdtest_debug

jpgdtest:
	$(CC) -DZJD_DEBUG=0 -DJD_DEBUG=0 -DJD_USE_SCALE=0 -DJD_FASTDECODE=1 -I tjpgd/src -I zjpgd/zjpgd main.c tjpgd/src/tjpgd.c zjpgd/zjpgd/zjpgd.c -o jpgdtest

jpgdtest_debug:
	$(CC) -DZJD_DEBUG=1 -DJD_DEBUG=1 -DJD_USE_SCALE=0 -DJD_FASTDECODE=1 -I tjpgd/src -I zjpgd/zjpgd main.c tjpgd/src/tjpgd.c zjpgd/zjpgd/zjpgd.c -o jpgdtest_debug

clean:
	rm -f jpgdtest*
