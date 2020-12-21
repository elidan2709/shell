COMPILER=gcc
FLAGS=-O2 -ftrapv -fsanitize=undefined -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self -Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith -Wtype-limits -Wempty-body -Wstrict-prototypes -Wold-style-definition -Wmissing-field-initializers -Wnested-externs -Wno-pointer-sign -std=gnu11 -lm 
FILES=run.c processing.c parser.c
TARGET=shell

main:
	$(COMPILER) $(FLAGS) $(FILES) main.c -o $(TARGET) -lm

main-sanitize:
	$(COMPILER) $(FLAGS) -fsanitize=address $(FILES) main.c -o $(TARGET) -lm
