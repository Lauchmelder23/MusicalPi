CC := gcc
CFLAGS := -Wall -g
LIBRARIES := -lpthread
TARGET := musicalpi

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c, out/%.o, $(SRCS))

ALL: $(TARGET)

$(TARGET): mkdir $(OBJS) 
	$(CC) -o out/$@ $(filter-out $<, $^) $(LIBRARIES)

mkdir:
	mkdir -p out

out/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -c $< 

clean:
	rm -rf $(TARGET) out

.PHONY: all clean
