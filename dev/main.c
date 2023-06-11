
#include <stdio.h>
#include <wiringPi.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define LEFT_PEDAL_PIN 17
#define RIGHT_PEDAL_PIN 26
#define SPEAKER_PIN 18

#define SHM_KEY 1234

int pedalPressed; // Indicateur de l'état de la pédale
double startMillis; // Temps de départ de la pédale en millisecondes
double startChronoMillis; // Temps de départ du chrono
int elapsedMillis; // Temps écoulé en millisecondes

double startDelay = 3000.0; // Délai d'attente pour le départ en millisecondes
int chronoStarted = 0; // Indicateur du démarrage du chrono

pthread_t pedalThread; // Thread pour la pédale
pthread_t chronoThread; // Thread pour le chrono
pthread_mutex_t mutex; // Mutex pour la synchronisation des threads

typedef struct {
    int pedalPressed;
    int elapsedMillis;
} SharedData;

SharedData* openSharedMemory() {
    int shmid;
    key_t key = SHM_KEY;
    SharedData* sharedData;

    // Création de la mémoire partagée
    if ((shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666)) < 0) {
        perror("Erreur lors de la création de la mémoire partagée");
        pthread_exit(NULL);
    }

    // Attacher la mémoire partagée
    if ((sharedData = (SharedData*)shmat(shmid, NULL, 0)) == (SharedData*)-1) {
        perror("Erreur lors de l'attachement de la mémoire partagée");
        pthread_exit(NULL);
    }

    return sharedData;
}

void initVariables() {
    // Réinitialiser les variables pour la prochaine course
    pedalPressed = 0;
    startMillis = 0;
    startChronoMillis = 0;
    elapsedMillis = 0;
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
    SharedData* sharedData = openSharedMemory();

    // Initialisation des données partagées
    sharedData->pedalPressed = 0;
    sharedData->elapsedMillis = 0;
    
    while (1) {
        int currentPedalState = digitalRead(LEFT_PEDAL_PIN);

        if (!sharedData->pedalPressed && currentPedalState == HIGH) {
            // Pédale maintenue appuyée
            sharedData->pedalPressed = 1;

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

    // Détacher la mémoire partagée
    if (shmdt(sharedData) == -1) {
        perror("Erreur lors du détachement de la mémoire partagée");
    }

    pthread_exit(NULL);
}

void* startChrono(void* arg) {
    SharedData* sharedData = openSharedMemory();

    while (1) {
        pthread_mutex_lock(&mutex);
        int currentChronoStarted = chronoStarted;
        pthread_mutex_unlock(&mutex);

        if (currentChronoStarted) {
            int elapsedMillis = millis() - startChronoMillis;

            pthread_mutex_lock(&mutex);
            sharedData->elapsedMillis = elapsedMillis;
            pthread_mutex_unlock(&mutex);
            
            int minutes = (elapsedMillis / 60000) % 60;
            int seconds = (elapsedMillis / 1000) % 60;
            int centiseconds = (elapsedMillis / 10) % 100;

            printf("Temps écoulé : %02d:%02d:%02d\n", minutes, seconds, centiseconds);

            if (elapsedMillis >= 5000) {
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

    // Détacher la mémoire partagée
    if (shmdt(sharedData) == -1) {
        perror("Erreur lors du détachement de la mémoire partagée");
    }

    pthread_exit(NULL);
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

