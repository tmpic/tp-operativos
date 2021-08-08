RM=rm -rf build
CC=gcc

C_SRCS=$(shell find amonguscore -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:src/%.c=build/amonguscore/%.o)

# Clean and compile .so
all: build/libamonguscore.so

create-dirs:
	mkdir -p build/amonguscore

build/libamonguscore.so: create-dirs $(OBJS)
	$(CC) -shared -o "build/libamonguscore.so" $(OBJS)

build/amonguscore/%.o: amonguscore/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC += -DDEBUG -g
debug: all

# Clean build files
clean:
	$(RM) build

install: uninstall clean all
	cp -u build/libamonguscore.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include

uninstall:
	rm -f /usr/lib/libamonguscore.so
	rm -rf /usr/include/amonguscore

.PHONY: all create-dirs clean install uninstall