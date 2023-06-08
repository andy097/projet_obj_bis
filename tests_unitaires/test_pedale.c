#include <stdio.h>
#include <wiringPi.h>

#define PEDAL_PIN 21
#define SPEAKER_PIN 1
#define BUZZER_PIN 26

void playSound(int frequency, int duration) {
	int halfPeriod = 500000 / frequency; // Calcul de la demi-période en microsecondes
	int iterations = duration * 1000 / halfPeriod; // Calcul du nombre d'itérations pour la durée spécifiée

	for (int i = 0; i < iterations; i++) {
		digitalWrite(SPEAKER_PIN, HIGH);
		delayMicroseconds(halfPeriod);

		digitalWrite(SPEAKER_PIN, LOW);
		delayMicroseconds(halfPeriod);
	}
}

int main(void) {
	wiringPiSetupGpio();

	pinMode(PEDAL_PIN, INPUT);
//	pinMode(SPEAKER_PIN, OUTPUT);
//	pinMode(BUZZER_PIN, INPUT);
//	pullUpDnControl(BUZZER_PIN, PUD_UP);

	printf("Attente d'un grimpeur souhaitant s'entraîner...\n");

	int pedalPressed = 0; // Indicateur de l'état de la pédale
	double startMillis = 0; // Temps de départ en millisecondes
	double currentMillis = 0; // Temps actuel en millisecondes
	int startDelay = 3000; // Délai d'attente pour le départ en millisecondes
	double startChronoMillis = 0; // Temps de départ du chrono en millisecondes
	int chronoStarted = 0; // Indicateur du démarrage du chrono
	double elapsedSeconds = 0.0;

	while (1) {
		int pedalState = digitalRead(PEDAL_PIN);
		int buzzerState = digitalRead(BUZZER_PIN);
    printf("etat : %d\n", pedalState);
		// Préparation au départ
		if (!pedalPressed && pedalState == HIGH) {
			pedalPressed = 1; // La pédale est maintenue appuyée

			printf("Attention !\n");

			// Joue un son court grave à la 1ère seconde
			delay(3000);
			playSound(300, 200);
			printf("À vos marques...\n");

			// Joue un son court grave à la 2e seconde
			delay(1000);
			playSound(300, 200);
			printf("Prêts...\n");

			// Mémorise le temps de départ
			startMillis = millis();
		}

		// Faux départ
		if (pedalPressed && pedalState == LOW) {
			pedalPressed = 0; // La pédale a été relachée

			// Si la pédale est relachée avant le 3e son, c'est un faux départ
			double elapsedMillis = millis() - startMillis;
			if (elapsedMillis < startDelay) {
				// Joue un son différent pour indiquer un faux départ
				playSound(100, 500);
				printf("Faux départ !\n");
				break;
			}
		}

		// Départ valide
		if (pedalPressed) {
			// Vérifie si le délai de 3 secondes est écoulé
			currentMillis = millis();
			double elapsedMillis = currentMillis - startMillis;

			if (elapsedMillis >= startDelay && !chronoStarted) {
				// Joue un son aigu pour indiquer le départ
				playSound(500, 500);
				printf("Partez !\n");

				// Démarre le chrono
				startChronoMillis = millis();
    			chronoStarted = 1;
			}
		}

		// Course terminée
		if (buzzerState == HIGH) {
			// Bouton stop appuyé, arrêter le chrono
			elapsedSeconds = (millis() - startChronoMillis) / 1000;
			chronoStarted = 0;
			printf("Chrono arrêté !\n");
			printf("Votre temps : %.2f s\n", elapsedSeconds);

			// Jouer un nouveau son avec l'enceinte
			playSound(300, 100);
			playSound(400, 100);
			playSound(500, 200);
			break;
		}

		if (chronoStarted) {
			elapsedSeconds = (millis() - startChronoMillis) / 1000.0;
			printf("Temps écoulé : %.2f s\n", elapsedSeconds);
		}

		delay(10);
	}

	return 0;
}
