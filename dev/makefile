CC = gcc
CFLAGS = -Wall -Wextra 
WIRINGPI = -lwiringPi
PTHREAD = -pthread
LRT = -lrt
LM = -lm
GTK_FLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

MAIN_BOTTOM = main_bottom_final
DISPLAY = display_final

$(MAIN_BOTTOM): $(MAIN_BOTTOM).c
	$(CC) $(CFLAGS) $(GTK_FLAGS) $(MAIN_BOTTOM).c -o $(MAIN_BOTTOM) $(GTK_LIBS) $(WIRINGPI) $(PTHREAD) $(LRT)

$(DISPLAY): $(DISPLAY).c
	$(CC) $(CFLAGS) $(GTK_FLAGS) $(DISPLAY).c -o $(DISPLAY) $(GTK_LIBS) $(LRT) $(LM)
	
clean:
	rm -f $(MAIN_BOTTOM) $(DISPLAY)

