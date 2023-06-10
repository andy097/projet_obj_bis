#ifndef CLIMB_RACE_H
#define CLIMB_RACE_H

#define LEFT_PEDAL_PIN 17
#define RIGHT_PEDAL_PIN 26
#define SPEAKER_PIN 18

void initVariables();
void playSound(int frequency, int duration);
void* waitForPedal(void* arg);
void* startChrono(void* arg);

#endif
