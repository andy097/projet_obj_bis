CC = gcc
CFLAGS = -Wall -Wextra 
WIRINGPI = -lwiringPi
PTHREAD = -pthread
LRT = -lrt
LM = -lm
GTK_FLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

MAIN = main
DISPLAY = display

all: $(MAIN)

$(MAIN): $(MAIN).c
	$(CC) $(CFLAGS) $(GTK_FLAGS) $(MAIN).c -o $(MAIN) $(GTK_LIBS) $(WIRINGPI) $(PTHREAD) $(LRT)

$(DISPLAY): $(DISPLAY).c
	$(CC) $(CFLAGS) $(GTK_FLAGS) $(DISPLAY).c -o $(DISPLAY) $(GTK_LIBS) $(LRT) $(LM)
	
clean:
	rm -f $(MAIN) $(DISPLAY)

