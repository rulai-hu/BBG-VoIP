# Header files
HEADERS = \
	include/socket.h \
	include/ringbuffer.h

# Object files
OBJECTS = \
	socket.o \
	main.o \
	ringbuffer.o \
	libportaudio.a

# Compiler
COMPILER = arm-linux-gnueabihf-gcc

# GCC compilation flags
GCCFLAGS = -c -g -pthread -std=c99 -D _POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wmissing-declarations

GCCLINKERFLAGS = -pthread

# Output filename
OUTPUT = tincanphone
OUTPUT_DIR = $(HOME)/cmpt433/public/myApps/

LFLAGS = -lm -L$(HOME)/cmpt433/public/asound_lib_BBB -lasound

#####
###
#
# 'Make' commands
#
###
#####

# Executed whenever an object file is out of date
# Symbols:
#   % refers to any character(s)
#   $< refers to the first item in the dependency list
%.o: src/%.c $(HEADERS)
	$(COMPILER) $(GCCFLAGS) $<

# $ make all
all: $(OBJECTS)
	$(COMPILER) $(OBJECTS) $(GCCLINKERFLAGS) -o $(OUTPUT) $(LFLAGS)
	cp $(OUTPUT) $(OUTPUT_DIR)
	@echo "Program generated at: $(OUTPUT_DIR)$(OUTPUT)"

ringbuffer_test:
	$(COMPILER) src/main_ringbuffer.c src/ringbuffer.c -std=c99 -D _POSIX_C_SOURCE=200809L -o ringbuffer
	cp ringbuffer $(OUTPUT_DIR)

device_info:
	$(COMPILER) src/device_info.c libportaudio.a -pthread -std=c99 -D _POSIX_C_SOURCE=200809L -o device_info $(LFLAGS)
	cp device_info $(OUTPUT_DIR)

# $ make memchk
memchk: $(OBJECTS)
	make all
	valgrind --leak-check=full ./$(OUTPUT)

# $ make clean
# Removes created files
clean:
	rm -f $(OUTPUT) *.o *~
