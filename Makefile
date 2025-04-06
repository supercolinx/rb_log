
CC	:= gcc
TARGET	:= rbg

default: main.o rb_log.o
	@$(CC) -o $(TARGET) $^

main.o: main.c rb_log.h
	$(CC) -c $< -o $@

rb_log.o: rb_log.c rb_log.h
	$(CC) -c $< -o $@

clean:
	@rm -f *.o $(TARGET) *.log*

run: $(TARGET)
	@./$(TARGET) $(filter-out $@,$(MAKECMDGOALS))
