#include <stdio.h>
#include <wiringPi.h>

#define PEDAL_PIN 17
#define SPEAKER_PIN 18
#define BUZZER_PIN 17


int main(void) {
	wiringPiSetupGpio();
	
//	pinMode(PEDAL_PIN, INPUT);
//	pinMode(SPEAKER_PIN, OUTPUT);
	pinMode(BUZZER_PIN, INPUT);
	pullUpDnControl(BUZZER_PIN, PUD_UP);
	
	printf("Attente d'un grimpeur souhaitant s'entraîner...\n");

	
	while (1) {
		//int pedalState = digitalRead(PEDAL_PIN);
		int buzzerState = digitalRead(BUZZER_PIN);
		


		// Course terminée
		if (buzzerState == HIGH) {
			printf("Chrono arrêté !\n");
			//break;
		}
	
		
		delay(10);
	}
	
	return 0;
}
