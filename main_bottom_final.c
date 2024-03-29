#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <wiringPi.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg);exit(-1);}

#define PEDALG_PIN 29  //PIN de la pedale L
#define PEDALD_PIN 0  //PIN de la pedale R
#define BUZZER_PIN 1  //PIN du buzzer
#define DELAI_APPUI 3000 //Délai minimale de 3 secondes
#define SERVER_IP "192.168.1.199" //Adresse IP du serveur installé sur le RPI_MINI

int pedaleGAppuyePret=0,pedaleDAppuyePret=0;
pthread_mutex_t mutexPedaleG,mutexPedaleD,mutexlancementPartie,mutex;
int enJeuCours=0,enJeuCours2=0;
int ret=0;
double tempsEcoulePedaleL = 0.0,tempsEcoulePedaleD = 0.0;
double tempsActuelG = 0,  tempsActuelD = 0; // Temps actuel en millisecondes
int controle_break=0;
int mode_2_joueurs=0;
int mode_1_joueur=0, pedale_mode_1_joueur=0;
int pedaleGAppuyeParti=0,pedaleDAppuyeParti=0;
pthread_cond_t conditionLancementPartie;
int controleLancerPartie=0;

//Socket
int controleLancementSocket=0;
int messageEnvoyeSocket=0;
pthread_mutex_t mutexLancementSocket;
pthread_cond_t conditionLancementSocket;

//Structure du message à envoyer
struct message {
  int mode_jeu; //mode de jeu : 1 ou 2 joueurs
  int statut; //partie commencé ou terminé
  char message[100];
};

void emettreSon(int frequency, int duration) {
  int halfPeriod = 300000 / frequency; // Calcul de la demi-période en microsecondes
  int iterations = duration * 1000 / halfPeriod; // Calcul du nombre d'itérations pour la durée spécifiée

  for (int i = 0; i < iterations; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(halfPeriod / 2);

    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(halfPeriod / 2);
  }
}

void* gererPedaleG(void* arg) {

  double tempsDepart = 0; // Temps de départ en millisecondes
  int pedaleGAppuyee=0;
  while(1){

    int pedalState = digitalRead(PEDALG_PIN);
    // Préparation au départ
    if (!pedaleGAppuyee && pedalState == HIGH) {
      pthread_mutex_lock(&mutexPedaleG);
      pedaleGAppuyee = 1; // La pédale est maintenue appuyée
      printf("Grimpeur G  - Prêts ?\n");
      // Mémorise le temps de départ
      tempsDepart = millis();
      pthread_mutex_unlock(&mutexPedaleG);

    }

    if (pedaleGAppuyee && pedalState == HIGH) {
      // Vérifie si le délai de 3 secondes est écoulé
      pthread_mutex_lock(&mutexPedaleG);
      tempsActuelG = millis();
      pthread_mutex_unlock(&mutexPedaleG);
      double elapsedMillis = tempsActuelG - tempsDepart;
      if (elapsedMillis >= DELAI_APPUI) {
        pthread_mutex_lock(&mutexPedaleG);
        //  printf("Grimpeur G  - Prêts - Pedale G !\n");
        pedaleGAppuyePret=1;
        pthread_mutex_unlock(&mutexPedaleG);
      }
    }

    //Si la pédaleG est relachée, on réinistalise le temps de départ
    if (pedaleGAppuyee && pedalState == LOW) {
      pthread_mutex_lock(&mutexPedaleG);
      tempsDepart = 0; // Réinitialise le temps de départ
      pedaleGAppuyee = 0; // La pédale a été relâchée
      pedaleGAppuyePret=0; // La pédale appuyé après délai est invalide
      pthread_mutex_unlock(&mutexPedaleG);
      printf("Grimpeur G  - pédale G relaché !\n");
    }

    if (mode_1_joueur){
      if(!pedaleGAppuyePret && pedale_mode_1_joueur==1){
        pthread_mutex_lock(&mutexPedaleG);
        printf("Grimpeur G  - Non Prêt - Pedale G !\n");
        mode_1_joueur=0;
        pedale_mode_1_joueur=0;
        pedaleGAppuyePret=0;
        controle_break=1;
        pthread_mutex_unlock(&mutexPedaleG);
      }
    }
    if(pedaleGAppuyePret && pedaleDAppuyePret){
      pthread_mutex_lock(&mutexPedaleG);
      mode_2_joueurs=1;
      mode_1_joueur=0;
      pedale_mode_1_joueur=0;
      pthread_mutex_unlock(&mutexPedaleG);
    }
    if (mode_2_joueurs && (!pedaleGAppuyePret || !pedaleDAppuyePret)) {
      pthread_mutex_lock(&mutexPedaleG);
      printf("Grimpeur G - mode 2 joueurs - Non Prêt \n" );
      pedaleGAppuyePret=0;
      pedaleDAppuyePret=0;
      mode_1_joueur=0;
      pedale_mode_1_joueur=0;
      mode_2_joueurs=0;
      controle_break=1;
      pthread_mutex_unlock(&mutexPedaleG);
    }

  }
}

void* gererPedaleD(void* arg) {

  double tempsDepart = 0; // Temps de départ en millisecondes
  int pedaleDAppuyee = 0;
  while(1){

    int pedalState = digitalRead(PEDALD_PIN);
    // Préparation au départ
    if (!pedaleDAppuyee && pedalState == HIGH) {
      pthread_mutex_lock(&mutexPedaleD);
      pedaleDAppuyee = 1; // La pédale est maintenue appuyée
      printf("Grimpeur D  - Prêt ?\n");
      // Mémorise le temps de départ
      tempsDepart = millis();
      pthread_mutex_unlock(&mutexPedaleD);
    }

    if (pedaleDAppuyee && pedalState == HIGH) {
      // Vérifie si le délai de 3 secondes est écoulé
      pthread_mutex_lock(&mutexPedaleD);
      tempsActuelD = millis();
      pthread_mutex_unlock(&mutexPedaleD);
      double elapsedMillis = tempsActuelD - tempsDepart;
      if (elapsedMillis >= DELAI_APPUI) {
        pthread_mutex_lock(&mutexPedaleD);
        pedaleDAppuyePret = 1;
        pthread_mutex_unlock(&mutexPedaleD);
      }
    }

    // Si la pédale D est relâchée, on réinitialise le temps de départ
    if (pedaleDAppuyee && pedalState == LOW) {
      printf("Grimpeur D  - pédale D relâchée !\n");
      pthread_mutex_lock(&mutexPedaleD);
      tempsDepart = 0; // Réinitialise le temps de départ
      pedaleDAppuyee = 0; // La pédale a été relâchée
      pedaleDAppuyePret = 0; // La pédale appuyée après délai est invalide
      pthread_mutex_unlock(&mutexPedaleD);
    }

    if (mode_1_joueur){
      if(!pedaleDAppuyePret && pedale_mode_1_joueur==2){
        pthread_mutex_lock(&mutexPedaleD);
        printf("Grimpeur D  - Non Prêt - Pedale D !\n");
        mode_1_joueur=0;
        pedale_mode_1_joueur=0;
        pedaleDAppuyePret=0;
        controle_break=1;
        pthread_mutex_unlock(&mutexPedaleD);

      }
    }
    if(pedaleGAppuyePret && pedaleDAppuyePret){
      pthread_mutex_lock(&mutexPedaleD);
      mode_2_joueurs=1;
      mode_1_joueur=0;
      pthread_mutex_unlock(&mutexPedaleD);
    }

    if (mode_2_joueurs && (!pedaleGAppuyePret || !pedaleDAppuyePret)) {
      pthread_mutex_lock(&mutexPedaleD);
      printf("Grimpeur D - mode 2 joueurs - Non Prêt \n" );
      pedaleGAppuyePret=0;
      pedaleDAppuyePret=0;
      mode_2_joueurs=0;
      pedale_mode_1_joueur=0;
      mode_1_joueur=0;
      controle_break=1;
      pthread_mutex_unlock(&mutexPedaleD);
    }

  }
}


void* demarrerJeu(void* arg) {
  while(1){
    if (pedaleGAppuyePret || pedaleDAppuyePret) {
      controle_break=0;
      if(pedaleGAppuyePret){
        pthread_mutex_lock(&mutexPedaleG);
        printf("Grimpeur G  - Prêt - Pedale G !\n");
        mode_1_joueur=1;
        pedale_mode_1_joueur=1;
        pthread_mutex_unlock(&mutexPedaleG);
      }
      if(pedaleDAppuyePret){
        pthread_mutex_lock(&mutexPedaleD);
        printf("Grimpeur D  - Prêt - Pedale D !\n");
        mode_1_joueur=1;
        pedale_mode_1_joueur=2;
        pthread_mutex_unlock(&mutexPedaleD);
      }
      if(pedaleGAppuyePret && pedaleDAppuyePret){
        mode_2_joueurs=1;
      }
      int duree=100;
      for (int i = 0; i < 3; i++) {
        printf("i %d\n",i );

        emettreSon(500,duree);
        duree+=300;
        printf("pedale_mode_1_joueur %d !\n",pedale_mode_1_joueur);
        printf("pedaleGAppuyePret %d !\n",pedaleGAppuyePret);
        printf("pedaleDAppuyePret %d !\n",pedaleDAppuyePret);
        printf("mode_2_joueurs %d !\n",mode_2_joueurs);
        printf("enJeuCours %d !\n",enJeuCours);
        delay(2000);
        if(controle_break){
          enJeuCours=0;
          break;
        }
        enJeuCours=1;

      }
      if(enJeuCours){
        pthread_mutex_lock(&mutexlancementPartie);
        controleLancerPartie=1;
        printf("HELLO\n");
        pthread_cond_signal(&conditionLancementPartie);
        pthread_mutex_unlock(&mutexlancementPartie);

      }
    }
  }
}

int lancer_client_socket(int port, struct message *msg) {
  int retour=-1;
  int client_socket;
  struct sockaddr_in addresse_serveur;

  // Créer le socket client
  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    printf("Erreur lors de la création du socket\n");
  }

  // Préparer la structure sockaddr_in pour le serveur
  addresse_serveur.sin_family = AF_INET;
  addresse_serveur.sin_addr.s_addr = inet_addr(SERVER_IP);
  addresse_serveur.sin_port = htons(port);

  // Établir une connexion avec le serveur
  if (connect(client_socket, (struct sockaddr*)&addresse_serveur, sizeof(addresse_serveur)) < 0) {
    perror("Erreur lors de la connexion au serveur\n");
  }

  // La connexion au serveur a réussi
  printf("Connecté au serveur\n");

  // Envoyer le message structuré au serveur
  if (send(client_socket, msg, sizeof(struct message), 0) < 0) {
    perror("Erreur lors de l'envoi du message\n");
    //  return 1;
  }

  printf("Message envoyé avec succès\n");

  // Attendre la réponse du serveur
  struct message response;
  ssize_t received_bytes = recv(client_socket, &response, sizeof(struct message), 0);
  if (received_bytes < 0) {
    perror("Erreur lors de la réception de la réponse du serveur\n");
  } else if (received_bytes == 0) {
    printf("La connexion avec le serveur a été fermée\n");

    retour= 1;
  }

  // Fermer le socket client
  if (close(client_socket) == -1) {
    perror("Erreur lors de la fermeture du socket client");
  } else {
    printf("Socket client fermé avec succès\n");
    // Cast du pointeur du message reçu vers la structure appropriée
    struct message* received_message = (struct message*)&response;

    if (strcmp(received_message->message, "pedaleL-Callback") == 0) {
      pthread_mutex_lock(&mutexPedaleG);
      pedaleGAppuyePret = 0;
      pthread_mutex_unlock(&mutexPedaleG);
      ret=1;
      enJeuCours=0;
      enJeuCours2=0;
      printf("pedaleL-Callback\n");

    }
    if (strcmp(received_message->message, "pedaleR-Callback") == 0) {

    }
    if (strcmp(received_message->message, "partie2Joueurs-Callback") == 0) {
    }
  }
  return retour;
}

void* lancerSocketClient(void* arg) {
  // Créer un message à envoyer au serveur
  pthread_mutex_lock(&mutexLancementSocket);
  int lancement_SocketPedaleGMode1Joueur=0,lancement_SocketPedaleDMode1Joueur=0,lancement_SocketPedalesMode2Joueurs=0;

  struct message msg_PedaleGMode1Joueur;
  msg_PedaleGMode1Joueur.mode_jeu = 1;
  msg_PedaleGMode1Joueur.statut = 0;
  strcpy(msg_PedaleGMode1Joueur.message, "pedaleL");

  struct message msg_PedaleDMode1Joueur;
  msg_PedaleDMode1Joueur.mode_jeu = 1;
  msg_PedaleDMode1Joueur.statut = 0;
  strcpy(msg_PedaleDMode1Joueur.message, "pedaleR");

  struct message msg_PedalesMode2Joueurs;
  msg_PedalesMode2Joueurs.mode_jeu = 2;
  msg_PedalesMode2Joueurs.statut = 0;
  strcpy(msg_PedalesMode2Joueurs.message, "NULL");


  // Appeler la fonction lancer_client_socket en spécifiant le port et le message
  int port = 8080;

  while (controleLancementSocket == 0) {
    pthread_cond_wait(&conditionLancementSocket, &mutexLancementSocket);
  }

  switch (messageEnvoyeSocket) {
    case 1:
    lancement_SocketPedaleGMode1Joueur=lancer_client_socket(port, &msg_PedaleGMode1Joueur);
    break;
    case 2:
    lancement_SocketPedaleDMode1Joueur=lancer_client_socket(port, &msg_PedaleDMode1Joueur);
    break;
    case 3:
    lancement_SocketPedalesMode2Joueurs=lancer_client_socket(port, &msg_PedalesMode2Joueurs);
    break;
  }
  pthread_mutex_unlock(&mutexLancementSocket);
}

void* lancerPartie(void* arg) {
  pthread_mutex_lock(&mutexlancementPartie);
  while (!controleLancerPartie) {
    pthread_cond_wait(&conditionLancementPartie, &mutexlancementPartie);
  }
  pthread_mutex_unlock(&mutexlancementPartie);
  const int pedale_mode_1_joueur_stockee = pedale_mode_1_joueur;
  const int mode_2_joueurs_stockee = mode_2_joueurs;
  enJeuCours2=1;
  controleLancerPartie=0;
  while(enJeuCours2) {
    if(pedale_mode_1_joueur_stockee==1){
      messageEnvoyeSocket=1;
      controleLancementSocket=1;
      pthread_cond_signal(&conditionLancementSocket);   // Déclencher le signal de la condition
      tempsEcoulePedaleL = (millis() - tempsActuelG) / 1000.0;
      printf("Temps écoulé - Pedale G : %.2f s\n", tempsEcoulePedaleL);
    }
    if(pedale_mode_1_joueur_stockee==2){

      messageEnvoyeSocket=2;
      pthread_cond_signal(&conditionLancementSocket);   // Déclencher le signal de la condition
      tempsEcoulePedaleD = (millis() - tempsActuelD) / 1000.0;
      printf("Temps écoulé - Pedale D : %.2f s\n", tempsEcoulePedaleD);
    }
    if(mode_2_joueurs_stockee){
      messageEnvoyeSocket=3;
      pthread_cond_signal(&conditionLancementSocket);   // Déclencher le signal de la condition

      tempsEcoulePedaleL = (millis() - tempsActuelG) / 1000.0;
      printf("Temps écoulé - Pedale G : %.2f s\n", tempsEcoulePedaleL);

      tempsEcoulePedaleD = (millis() - tempsActuelD) / 1000.0;
      printf("Temps écoulé - Pedale D : %.2f s\n", tempsEcoulePedaleD);

    }
  }
}

int main() {
  pthread_t thread_pedaleG, thread_pedaleD, thread_jeu, thread_socket_client,thread_execution_partie;

  CHECK(wiringPiSetup(),"Erreur lors de l'initialisation de WiringPi\n");
  pinMode(PEDALG_PIN, INPUT); // Configure la broche en mode entrée
  pinMode(PEDALD_PIN, INPUT);
  pullUpDnControl(PEDALD_PIN, PUD_UP);

  pthread_cond_init(&conditionLancementSocket, NULL);

  pthread_mutex_init(&mutexPedaleG, NULL);
  pthread_mutex_init(&mutexPedaleD, NULL);
  pthread_mutex_init(&mutexlancementPartie, NULL);
  pthread_mutex_init(&mutex, NULL);

  pthread_create(&thread_pedaleG, NULL, gererPedaleG, NULL);
  pthread_create(&thread_pedaleD, NULL, gererPedaleD, NULL);
  pthread_create(&thread_jeu, NULL, demarrerJeu, NULL);
  pthread_create(&thread_socket_client, NULL, lancerSocketClient, NULL);
  pthread_create(&thread_execution_partie, NULL, lancerPartie, NULL);

  while(1);

  pthread_join(thread_pedaleG, NULL);
  pthread_join(thread_pedaleD, NULL);
  pthread_join(thread_jeu, NULL);
  pthread_join(thread_socket_client, NULL);
  pthread_join(thread_execution_partie, NULL);

  pthread_cond_destroy(&conditionLancementSocket);

  pthread_mutex_destroy(&mutexPedaleG);
  pthread_mutex_destroy(&mutexPedaleD);
  pthread_mutex_destroy(&mutexlancementPartie);
  pthread_mutex_destroy(&mutex);
}
