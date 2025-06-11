include ./make.config

C_SOURCES = $(shell find src -type f -name "*.c")
DEBUG = 0

ifeq ($(DEBUG), 1)
CFLAGS += -g
endif

compile:
	$(CC) $(CFLAGS) $(C_SOURCES) -o $(TARGET)

execute:
	./$(TARGET)

clean:
	rm $(TARGET)
