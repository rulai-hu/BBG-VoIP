# Header files
HEADERS = \
	include/volume_mixer.h \
	include/file.h \
	include/gpio.h \
	include/i2c.h \

# C files
CODE = \
	src/volume_mixer.c \
	src/file.c \
	src/gpio.c \
	src/i2c.c

# Compiler
COMPILER = arm-linux-gnueabihf-gcc

# GCC compilation flags
GCCFLAGS = -c -g -I$(CURDIR) -pthread -std=c99 -D _POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wmissing-declarations

GCCLINKERFLAGS = -pthread

# Output filename
OUTPUT = volume_mixer
OUTPUT_DIR = $(HOME)/cmpt433/public/myApps/

LFLAGS = -lm -L$(HOME)/cmpt433/public/asound_lib_BBB -lasound -lpthread

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
all: volume_mixer systemd
systemd:
	cp volume_mixer.service $(OUTPUT_DIR)
volume_mixer: $(OBJECTS)
	$(COMPILER) $(CODE) $(HEADERS) $(GCCLINKERFLAGS) -o $(OUTPUT) $(LFLAGS)
	cp $(OUTPUT) $(OUTPUT_DIR)
	@echo "Program generated at: $(OUTPUT_DIR)$(OUTPUT)"

# $ make clean
# Removes created files
clean:
	rm -f $(OUTPUT) *.o *~
