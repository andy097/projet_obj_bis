#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>
#include <wiringPi.h>
#define PORT 8080

#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg);exit(-1);}

#define MAX_CLIENTS 2
#define BOUTONG_PIN 25  //PIN de lu bouton G
#define BOUTOND_PIN 0  //PIN de la bouton D

//Structure du message à envoyer
struct message {
  int mode_jeu;
  int statut;
  char message[100];
};

#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg);exit(-1);}

/**************************************************************************************************/
/*Fonction : traiter_client                                                                          */
/* Description :   Nous  gérons la communication avec un client connecté au serveur, en recevant un message du client,
en effectuant certaines actions en fonction du contenu du message. Si la pédale est cliqué,
nous renvoyons une réponse au client si nécessaire.                                               */
/**************************************************************************************************/
void* traiter_client(void* client_socket_ptr) {
  int client_socket = *((int*)client_socket_ptr);
  free(client_socket_ptr);


  // Nous réceptionnons le message structuré du client
  struct message message_recu;
  if (recv(client_socket, &message_recu, sizeof(message_recu), 0) < 0) {
    perror("Erreur lors de la réception du message\n");
    return NULL;
  }

  //Si c'est le joueur G en mode 1 joueur
  if (strcmp(message_recu.message, "pedaleG") == 0 && message_recu.mode_jeu == 1) {
    printf("Le contenu correspond à 'pedaleG' en mode 1 joueur\n");
    printf("Grimpeur G  - Prêt ?\n");
    while(1){
      // Nous préparons le message de réponse au client
      struct message message_reponse_serveur;
      message_reponse_serveur.mode_jeu = 1;
      message_reponse_serveur.statut = 1;
      strcpy(message_reponse_serveur.message, "pedaleG-Callback");

      int pedalState = digitalRead(BOUTONG_PIN);
      //Si la pédale est cliquée, nous envoyons le message au client
      if (pedalState == HIGH) {

        if (send(client_socket, &message_reponse_serveur, sizeof(message_reponse_serveur), 0) < 0) {
          perror("Erreur lors de l'envoi du message de callback\n");
          return -1;
        }
        printf("Callback pedaleG envoyé\n");
        break;
      }
    }
  }

  //Si c'est le joueur D en mode 1 joueur
  if ((strcmp(message_recu.message, "pedaleD") == 0 && message_recu.mode_jeu == 1)) {
    printf("Le contenu correspond à 'pedaleD' en mode 1 joueur\n");

    double tempsDepart = 0; // Temps de départ en millisecondes
    int pedaleDAppuyee = 0;
    printf("Grimpeur D  - Prêt ?\n");
    while(1){
      // Nous préparons le message de réponse au client
      struct message message_reponse_serveur;
      message_reponse_serveur.mode_jeu = 1;
      message_reponse_serveur.statut = 1;
      strcpy(message_reponse_serveur.message, "pedaleD-Callback");

      int pedalState = digitalRead(BOUTOND_PIN);
      //Si la pédale est cliquée, nous envoyons le message au client
      if (pedalState == HIGH) {
        if (send(client_socket, &message_reponse_serveur, sizeof(message_reponse_serveur), 0) < 0) {
          perror("Erreur lors de l'envoi du message de callback\n");
          return -1;
        }

        printf("Callback pedaleD envoyé\n");
        break;

      }


    }
  }


  //Si c'est le joueur D en mode 2 joueurs
  if ((strcmp(message_recu.message, "pedaleD") == 0 && message_recu.mode_jeu == 2)) {
    printf("Le contenu correspond à 'pedaleD' en mode 2 joueurs\n");

    double tempsDepart = 0; // Temps de départ en millisecondes
    int pedaleDAppuyee = 0;
    printf("Grimpeur D  - Prêt ?\n");


    // Nous préparons le message de réponse au client
    struct message message_reponse_serveur;
    message_reponse_serveur.mode_jeu = 2;
    message_reponse_serveur.statut = 1;
    strcpy(message_reponse_serveur.message, "pedaleD-Callback");

    int pedalState = digitalRead(BOUTOND_PIN);
    //Si la pédale est cliquée, nous envoyons le message au client
    if (pedalState == HIGH) {

      if (send(client_socket, &message_reponse_serveur, sizeof(message_reponse_serveur), 0) < 0) {
        perror("Erreur lors de l'envoi du message de callback\n");
        return -1;
      }
      printf("Callback pedaleG - mode 2 joueurs envoyé\n");
      break;
    }


    return 0;
  }

  //Si c'est le joueur G en mode 2 joueurs
  if ((strcmp(message_recu.message, "pedaleG") == 0 && message_recu.mode_jeu == 2)) {
    printf("Le contenu correspond à 'pedaleG' en mode 2 joueurs\n");

    double tempsDepart = 0; // Temps de départ en millisecondes
    int pedaleDAppuyee = 0;
    printf("Grimpeur G  - Prêt ?\n");
    while(1){

      // Nous préparons le message de réponse au client
      struct message message_reponse_serveur;
      message_reponse_serveur.mode_jeu = 2;
      message_reponse_serveur.statut = 1;
      strcpy(message_reponse_serveur.message, "pedaleG-Callback");

      int pedalState = digitalRead(BOUTONG_PIN);
      //Si la pédale est cliquée, nous envoyons le message au client
      if (pedalState == HIGH) {
        if (send(client_socket, &message_reponse_serveur, sizeof(message_reponse_serveur), 0) < 0) {
          perror("Erreur lors de l'envoi du message de callback\n");
          return -1;
        }
        printf("Callback pedaleG - mode 2 joueurs envoyé\n");
        break;
      }

    }
  }

  // Nous fermons la socket client
  close(client_socket);
  return NULL;
}

/**************************************************************************************************/
/*Fonction : lancer_serveur_socket                                                                  */
/* Description :   Nous créons un socket de serveur et le lions à une adresse IP et à un port spécifiés.
Le serveur réceptionne les connexions entrantes des clients                                        */
/**************************************************************************************************/
void* lancer_serveur_socket(void* port_ptr) {
  intptr_t port = (intptr_t)port_ptr;

  struct sockaddr_in server_address, client_address;
  int server_socket, client_socket, c;

  //Nous créons le socket serveur
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    printf("Erreur lors de la création du socket\n");
    return NULL;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  //Nous lions le socket à l'adresse et au port
  if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
    perror("Erreur lors du bind du socket\n");
    return NULL;
  }

 //Nous mettons au socket serveur en mode d'écoute des connexions entrantes  clientes
  if (listen(server_socket, MAX_CLIENTS) < 0) {
    perror("Erreur lors de la mise en écoute du socket\n");
    return NULL;
  }

  printf("Serveur en attente de connexions...\n");

  c = sizeof(struct sockaddr_in);

  while (1) {
    //Le serveur accepte la connexion entrante du client
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&c);
    if (client_socket < 0) {
      perror("Erreur lors de l'acceptation de la connexion\n");
      continue;
    }

    printf("Nouvelle connexion acceptée\n");

    // Nous créons un thread spécifique à chaque connexion cliente
    pthread_t client_thread;
    int* client_socket_ptr = malloc(sizeof(int));
    *client_socket_ptr = client_socket;
    pthread_create(&client_thread, NULL, traiter_client, (void*)client_socket_ptr);

    // Nous détachons le thread de la connexion cliente
    pthread_detach(client_thread);
  }

  close(server_socket);
  return NULL;
}



int main(int argc, char const* argv[]) {
  pthread_t server_thread;
  intptr_t port = PORT;
  CHECK(wiringPiSetup(),"Erreur lors de l'initialisation de WiringPi\n");
  pinMode(BOUTOND_PIN, INPUT); // Nous configurons la broche  du bouton en mode entrée
  pinMode(BOUTONG_PIN, INPUT);  // Nous configurons la broche  du bouton en mode entrée
  pullUpDnControl(BOUTONG_PIN, PUD_UP);
  pullUpDnControl(BOUTOND_PIN, PUD_UP);
  pthread_create(&server_thread, NULL, lancer_serveur_socket, (void*)port);

  while (1);
  pthread_join(server_thread, NULL);

  return 0;
}
