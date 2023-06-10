#include <stdio.h>
#include <wiringPi.h>
#include <pthread.h>
#include "climb_race.h"

int pedalPressed; // Indicateur de l'état de la pédale
double startMillis; // Temps de départ de la pédale en millisecondes
double startChronoMillis; // Temps de départ du chrono
double elapsedSeconds;

double startDelay = 3000.0; // Délai d'attente pour le départ en millisecondes
int chronoStarted = 0; // Indicateur du démarrage du chrono

pthread_t pedalThread; // Thread pour la pédale
pthread_t chronoThread; // Thread pour le chrono
pthread_mutex_t mutex; // Mutex pour la synchronisation des threads

void initVariables() {
    // Réinitialiser les variables pour la prochaine course
    pedalPressed = 0;
    startMillis = 0;
    startChronoMillis = 0;
    elapsedSeconds = 0.0;
    chronoStarted = 0;

    printf("Attente d'un grimpeur souhaitant s'entraîner...\n");
}

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

void* waitForPedal(void* arg) {
    while (1) {
        int currentPedalState = digitalRead(LEFT_PEDAL_PIN);

        if (!pedalPressed && currentPedalState == HIGH) {
            // Pédale maintenue appuyée
            pedalPressed = 1;

            printf("Pédale pressée !\n");
            delay(3000);
            startMillis = millis();
            playSound(300, 200);
            printf("À vos marques...\n");
            delay(1000);
            playSound(300, 200);
            printf("Prêts...\n");
            delay(1000);

            pthread_mutex_lock(&mutex);
            startChronoMillis = millis();
            chronoStarted = 1;
            pthread_mutex_unlock(&mutex);
            
			printf("Partez !\n");
			playSound(500, 200);
			
			break;
        }
    }

    pthread_exit(NULL);
}

void* startChrono(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        int currentChronoStarted = chronoStarted;
        pthread_mutex_unlock(&mutex);

        if (currentChronoStarted) {
            double elapsedSeconds = (millis() - startChronoMillis) / 1000.0;

            pthread_mutex_lock(&mutex);
            elapsedSeconds = elapsedSeconds;
            pthread_mutex_unlock(&mutex);

            printf("Temps écoulé : %.2f s\n", elapsedSeconds);

            if (elapsedSeconds >= 5.0) {
                pthread_mutex_lock(&mutex);
                chronoStarted = 0;
                pthread_mutex_unlock(&mutex);

                printf("Chrono arrêté !\n");

                playSound(300, 100);
                playSound(400, 100);
                playSound(500, 200);

                initVariables();
                break;
            }
        }

        delay(10);
    }
}

int main(void) {
    wiringPiSetupGpio();

    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(LEFT_PEDAL_PIN, INPUT);

    initVariables();

    // Initialisation du mutex
    pthread_mutex_init(&mutex, NULL);
    
    while (1) {
		// Création du thread pour la pédale
		pthread_create(&pedalThread, NULL, waitForPedal, NULL);

		// Thread pour le chrono
		pthread_create(&chronoThread, NULL, startChrono, NULL);

		// Attendre la fin des threads
		pthread_join(pedalThread, NULL);
		pthread_join(chronoThread, NULL);
	}
	
	// Destruction du mutex
	pthread_mutex_destroy(&mutex);

    return 0;
}

