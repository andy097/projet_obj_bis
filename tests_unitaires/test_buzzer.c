#include <wiringPi.h>

#define BUZZER_PIN 1 // GPIO 17 (wiringPi numbering)

int main(void) {
    // Initialisation de wiringPi
    if (wiringPiSetup() == -1) {
        return 1; // Erreur lors de l'initialisation
    }

    // Configuration de la broche du buzzer en mode sortie
    pinMode(BUZZER_PIN, OUTPUT);

    // Allumer le buzzer en alternant rapidement entre HIGH et LOW
    while (1) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100); // Temps pendant lequel le buzzer est allumé
        digitalWrite(BUZZER_PIN, LOW);
        delay(100); // Temps pendant lequel le buzzer est éteint
    }

    return 0;
}

