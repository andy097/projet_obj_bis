CC = gcc
CFLAGS = -Wall -Wextra 
WIRINGPI = -lwiringPi
PTHREAD = -pthread
GTK_FLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

MAIN = climb_race
DISPLAY = display

all: $(MAIN)

$(MAIN): $(MAIN).c $(MAIN).h
	$(CC) $(CFLAGS) $(PTHREAD) $(MAIN).c -o $(MAIN) $(WIRINGPI)

$(DISPLAY): $(DISPLAY).c
	$(CC) $(CFLAGS) $(GTK_FLAGS) $(DISPLAY).c -o $(DISPLAY) $(GTK_LIBS)
	
clean:
	rm -f $(MAIN) $(DISPLAY)

