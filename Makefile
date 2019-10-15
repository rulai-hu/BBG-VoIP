# Header files
HEADERS = 

# Object files
OBJECTS = \
	main.o

# Compiler
COMPILER = arm-linux-gnueabihf-gcc

# GCC compilation flags
GCCFLAGS = -c -g -pthread -std=c99 -D _POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wmissing-declarations

GCCLINKERFLAGS = -pthread

# Output filename
OUTPUT = tincanphone
OUTPUT_DIR = $(HOME)/cmpt433/public/myApps/

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
	$(COMPILER) $(OBJECTS) $(GCCLINKERFLAGS) -o $(OUTPUT)
	cp $(OUTPUT) $(OUTPUT_DIR)
	@echo "Program generated at: $(OUTPUT_DIR)$(OUTPUT)"

# $ make memchk
memchk: $(OBJECTS)
	make all
	valgrind --leak-check=full ./$(OUTPUT)

# $ make clean
# Removes created files
clean:
	rm -f $(OUTPUT) *.o *~
