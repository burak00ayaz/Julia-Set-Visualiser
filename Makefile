CFLAGS= -std=gnu11 -O2 -Wall -Wextra -Wpedantic

SOURCE_FILES=main.c naive.c performanz.c intrin_v0.c intrin_v1.c bmp.c util.c correctness.c

.PHONY: main

# -lm: link math library
main:
	cd src && gcc $(CFLAGS) -o ../julia $(SOURCE_FILES) -lm
