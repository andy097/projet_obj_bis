#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <wiringPi.h>

#define PEDAL_PIN_L 21  //PIN de la pedale L
#define PEDAL_PIN_R 17  //PIN de la pedale R

pthread_mutex_t mutex,mutex_PedaleR,mutex_Partie2Joueurs;
pthread_cond_t cond_L,cond_R,cond_partie2Joueurs;
int premierPedaleappuye=-1;
int demarrage_threadPartie2Joueurs = 0;

double debutChronoPedaleR = 0; // Temps de départ du chrono en millisecondes
double tempsEcoulePedaleR = 0.0;
double debutChronoPedaleL = 0; // Temps de départ du chrono en millisecondes
double tempsEcoulePedaleL = 0.0;

void* gererPedaleL(void* arg) {
  struct timespec timeout;
  struct timeval now;

  printf("Attente d'un grimpeur L souhaitant s'entraîner...\n");

  int pedalAppuye = 0; // Indicateur de l'état de la pédale
  double tempsDepart = 0; // Temps de départ en millisecondes
  double tempsActuel = 0; // Temps actuel en millisecondes
  int delaiAttente = 3000; // Délai d'attente pour le départ en millisecondes

  int chronoDemarrage = 0; // Indicateur du démarrage du chrono

  int retourThreadCondWait = 1;
  int compteur=1;

  while (1){

    //Initialisation du timer d'attente de la réponse de l'autre à 10s
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + 10;
    timeout.tv_nsec = now.tv_usec * 1000;

    int pedalState = digitalRead(PEDAL_PIN_L);

    // Préparation au départ
    if (!pedalAppuye && pedalState == HIGH) {
      pedalAppuye = 1; // La pédale est maintenue appuyée
      if(premierPedaleappuye==-1){
        premierPedaleappuye=0;
      }
      printf("Attention !\n");
      delay(1000);
      printf("À vos marques...\n");
      delay(1000);
      printf("Prêts...\n");

      // Mémorise le temps de départ
      tempsDepart = millis();

    }

    if (pedalAppuye && pedalState == LOW) {
      pedalAppuye = 0; // La pédale a été relâchée
      tempsDepart = 0; // Réinitialise le temps de départ
    }

    // Départ valide
    if (pedalAppuye) {
      // Vérifie si le délai de 3 secondes est écoulé
      tempsActuel = millis();
      double elapsedMillis = tempsActuel - tempsDepart;
      if (elapsedMillis >= delaiAttente && !chronoDemarrage) {
        pthread_cond_signal(&cond_R);
        pthread_mutex_lock(&mutex);
        printf("Partez - Pedale L !\n");
        pthread_mutex_unlock(&mutex);
        // Démarrage du chrono de la pedale L
        debutChronoPedaleL = millis();
        // Attendre le signal pendant 30 secondes ou jusqu'à expiration du délai
        retourThreadCondWait = pthread_cond_timedwait(&cond_L, &mutex, &timeout);
        chronoDemarrage = 1;
        debutChronoPedaleL = millis();
      }

    }
    if (retourThreadCondWait == 0 && compteur<=1 && premierPedaleappuye==0) {
      compteur++;
      printf("Signal reçu de la pedale R avant l'expiration du délai.\n");
      demarrage_threadPartie2Joueurs = 1; // Indicateur du lancement du threadPartie2Joueurs
      pthread_cond_signal(&cond_partie2Joueurs);

    }

    //Si le thread n'a pas recu de reponse de l'autre et le délai d'attente de 3 secondes est respecté
    if(chronoDemarrage && retourThreadCondWait!=0){
      tempsEcoulePedaleL = (millis() - debutChronoPedaleL) / 1000.0;
      printf("Temps écoulé - 1 joueur - Pedale L : %.2f s\n", tempsEcoulePedaleL);
    }

  }
  pthread_exit(NULL);
}



void* gererPedaleR(void* arg) {
  struct timespec timeout;
  struct timeval now;

  printf("Attente d'un grimpeur R souhaitant s'entraîner...\n");

  int pedalAppuye = 0; // Indicateur de l'état de la pédale
  double tempsDepart = 0; // Temps de départ en millisecondes
  double tempsActuel = 0; // Temps actuel en millisecondes
  int delaiAttente = 3000; // Délai d'attente pour le départ en millisecondes
  int chronoDemarrage = 0; // Indicateur du démarrage du chrono
  int retourThreadCondWait = 1;
  int compteurur=1;
  while (1){

    //Initialisation du timer d'attente de la réponse de l'autre à 10s
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + 10;
    timeout.tv_nsec = now.tv_usec * 1000;

    int pedalState = digitalRead(PEDAL_PIN_R);

    // Préparation au départ
    if (!pedalAppuye && pedalState == HIGH) {
      pedalAppuye = 1; // La pédale est maintenue appuyée
      if(premierPedaleappuye==-1){
        premierPedaleappuye=1;
      }
      printf("Attention !\n");
      delay(1000);
      printf("À vos marques...\n");
      delay(1000);
      printf("Prêts...\n");

      // Mémorise le temps de départ
      tempsDepart = millis();

    }

    if (pedalAppuye && pedalState == LOW) {
      pedalAppuye = 0; // La pédale a été relâchée
      tempsDepart = 0; // Réinitialise le temps de départ
    }

    // Départ valide
    if (pedalAppuye) {
      // Vérifie si le délai de 3 secondes est écoulé
      tempsActuel = millis();
      double elapsedMillis = tempsActuel - tempsDepart;
      //    printf("Le nombre est : %f\n", elapsedMillis);
      if (elapsedMillis >= delaiAttente && !chronoDemarrage) {
        pthread_cond_signal(&cond_L);
        pthread_mutex_lock(&mutex_PedaleR);
        printf("Partez - Pedale R !\n");
        pthread_mutex_unlock(&mutex_PedaleR);

        // Attendre le signal pendant 30 secondes ou jusqu'à expiration du délai
        retourThreadCondWait = pthread_cond_timedwait(&cond_R, &mutex_PedaleR, &timeout);
        chronoDemarrage = 1;
        debutChronoPedaleR = millis();

      }
    }

    if (retourThreadCondWait == 0 && compteurur<=1 && premierPedaleappuye==1) {
      compteur++;
      printf("Signal reçu de la pedale L avant l'expiration du délai.\n");
      demarrage_threadPartie2Joueurs = 1; // Indicateur du lancement du threadPartie2Joueurs
      pthread_cond_signal(&cond_partie2Joueurs);

    }
    //Si le thread n'a pas recu de reponse de l'autre et le délai d'attente de 3 secondes est respecté
    if(chronoDemarrage && retourThreadCondWait!=0){
      tempsEcoulePedaleR = (millis() - debutChronoPedaleR) / 1000.0;
      printf("Temps écoulé - 1 joueur - Pedale R : %.2f s\n", tempsEcoulePedaleR);
    }
  }
  pthread_exit(NULL);
}


void* partie2Joueurs(void* arg) {
  printf("Attente d'une partie de 2 joueurs...\n");

  pthread_mutex_lock(&mutex_Partie2Joueurs);
  while (demarrage_threadPartie2Joueurs == 0) {
    pthread_cond_wait(&cond_partie2Joueurs, &mutex_Partie2Joueurs)
  }
  // Démarrage du chrono de la pédale L
  debutChronoPedaleL = millis();
  // Démarrage du chrono de la pédale R
  debutChronoPedaleR = millis();

  while(1){
    tempsEcoulePedaleL = (millis() - debutChronoPedaleL) / 1000.0;
    printf("Temps écoulé - 2 joueurs - Pedale L : %.2f s\n", tempsEcoulePedaleL);
    tempsEcoulePedaleR = (millis() - debutChronoPedaleR) / 1000.0;
    printf("Temps écoulé - 2 joueurs -  Pedale R : %.2f s\n", tempsEcoulePedaleR);
  }

  pthread_mutex_unlock(&mutex_Partie2Joueurs);
}

int main() {
  pthread_t thread_pedale_L, thread_pedale_R, thread2Joueurs;
  wiringPi();
  pinMode(PEDAL_PIN_L, INPUT);
  pinMode(PEDAL_PIN_R, INPUT);
  pullUpDnControl(PEDAL_PIN_R, PUD_UP);

  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&mutex_PedaleR, NULL);
  pthread_mutex_init(&mutex_Partie2Joueurs, NULL);

  pthread_cond_init(&cond_L, NULL);
  pthread_cond_init(&cond_R, NULL);
  pthread_cond_init(&cond_partie2Joueurs, NULL);

  pthread_create(&thread_pedale_L, NULL, gererPedale1, NULL);
  pthread_create(&thread_pedale_R, NULL, gererPedaleR, NULL);
  pthread_create(&thread2Joueurs, NULL, partie2Joueurs, NULL);

  pthread_join(thread_pedale_L, NULL);
  pthread_join(thread_pedale_R, NULL);
  pthread_join(thread2Joueurs, NULL);

  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&mutex_PedaleR);
  pthread_mutex_destroy(&mutex_Partie2Joueurs);

  pthread_cond_destroy(&cond_L);
  pthread_cond_destroy(&cond_R);
  pthread_cond_destroy(&cond_partie2Joueurs);

  return 0;
}
