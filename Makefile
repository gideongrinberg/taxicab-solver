CC = clang++
CFLAGS = -std=c++17 -Wall -O3

TARGET = main
SRCS = main.cpp

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
