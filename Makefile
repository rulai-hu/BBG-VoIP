# Header files
HEADERS = \
	include/socket.h \
	include/ringbuffer.h \
	include/lfqueue.h \
	include/pink.h \
	include/portaudio.h \
	include/pa_linux_alsa.h

# Object files
OBJECTS = \
	socket.o \
	main.o \
	ringbuffer.o \
	libportaudio.a

# Compiler
COMPILER = arm-linux-gnueabihf-gcc

# GCC compilation flags
GCCFLAGS = -c -g -I$(CURDIR) -pthread -std=c99 -D _POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -Wmissing-declarations

GCCLINKERFLAGS = -pthread

# Output filename
OUTPUT = tincanphone
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
all: $(OBJECTS)
	$(COMPILER) $(OBJECTS) $(GCCLINKERFLAGS) -o $(OUTPUT) $(LFLAGS)
	cp $(OUTPUT) $(OUTPUT_DIR)
	@echo "Program generated at: $(OUTPUT_DIR)$(OUTPUT)"

pretty:
	$(COMPILER) src/main_pretty.c src/dialservice.c src/voiceserver.c src/addressbook.c src/call.c src/connection.c -I$(CURDIR) -g -std=c99 -D _POSIX_C_SOURCE=200809L -o pretty $(LFLAGS)
	mv pretty $(OUTPUT_DIR)

blocking_audio:
	$(COMPILER) src/main_blocking_audio.c src/lfqueue.c src/pink.c libportaudio.a -I$(CURDIR) -g -std=c99 -D _POSIX_C_SOURCE=200809L -o blocking_audio_test $(LFLAGS)
	mv blocking_audio_test $(OUTPUT_DIR)

nonblocking_audio:
	$(COMPILER) src/main_nonblocking_audio.c src/lfqueue.c src/pink.c src/lfringbuffer.c libportaudio.a -I$(CURDIR) -g -std=c99 -D _POSIX_C_SOURCE=200809L -o nonblocking_audio_test $(LFLAGS)
	mv nonblocking_audio_test $(OUTPUT_DIR)

lfqueue_test:
	$(COMPILER) src/main_lfqueue.c src/lfqueue.c -I$(CURDIR) -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -o lftest -lpthread -lm
	mv lftest $(OUTPUT_DIR)

pa_test:
	$(COMPILER) src/main_pa.c libportaudio.a -I$(CURDIR) -pthread -std=c99 -D _POSIX_C_SOURCE=200809L -o pa_test $(LFLAGS)
	cp pa_test $(OUTPUT_DIR)

ringbuffer_test:
	$(COMPILER) src/main_ringbuffer.c src/ringbuffer.c -I$(CURDIR) -std=c99 -D _POSIX_C_SOURCE=200809L -o ringbuffer
	cp ringbuffer $(OUTPUT_DIR)

lfringbuffer:
	$(COMPILER) src/main_lfringbuffer.c src/lfringbuffer.c -I$(CURDIR) -g -std=c99 -D _POSIX_C_SOURCE=200809L -o lfringbuffer $(LFLAGS)
	mv lfringbuffer $(OUTPUT_DIR)

device_info:
	$(COMPILER) src/device_info.c libportaudio.a -pthread -I$(CURDIR) -std=c99 -D _POSIX_C_SOURCE=200809L -o device_info $(LFLAGS)
	cp device_info $(OUTPUT_DIR)

# $ make memchk
memchk: $(OBJECTS)
	make all
	valgrind --leak-check=full ./$(OUTPUT)

# $ make clean
# Removes created files
clean:
	rm -f $(OUTPUT) *.o *~
