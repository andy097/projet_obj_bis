#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>

#define PORT 8080

struct message {
  int mode_jeu;
  int statut;
  char message[100];
};

int lancer_serveur_socket(int port) {

  struct termios originalAttr, newAttr;

  // Obtient les attributs du terminal actuels
  tcgetattr(STDIN_FILENO, &originalAttr);
  newAttr = originalAttr;

  // Configure le mode non canonique
  newAttr.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newAttr);

  char ch;

  int server_socket, client_socket, c;
  struct sockaddr_in server_address, client_address;

  // Créer le socket du serveur
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    printf("Erreur lors de la création du socket\n");
    return -1;
  }

  // Préparer la structure sockaddr_in pour le serveur
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  // Lier le socket à l'adresse et au port
  if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
    perror("Erreur lors du bind du socket\n");
    return -1;
  }

  // Mettre le socket en mode écoute
  if (listen(server_socket, 5) < 0) {
    perror("Erreur lors de la mise en écoute du socket\n");
    return -1;
  }

  printf("Serveur en attente de connexions...\n");

  c = sizeof(struct sockaddr_in);

  while (1) {
    // Accepter une connexion entrante
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&c);
    if (client_socket < 0) {
      perror("Erreur lors de l'acceptation de la connexion\n");
      continue;
    }

    printf("Nouvelle connexion acceptée\n");

    // Réception du message structuré
    struct message received_message;
    if (recv(client_socket, &received_message, sizeof(received_message), 0) < 0) {
      perror("Erreur lors de la réception du message\n");
      return -1;
    }

    if (strcmp(received_message.message, "pedaleL") == 0 && received_message.mode_jeu == 1) {
      printf("Le contenu correspond à 'pedaleL' en mode 1 joueur \n");

      // Préparer le message de callback
      struct message callback_message;
      callback_message.mode_jeu = 1;
      callback_message.statut = 1;
      strcpy(callback_message.message, "pedaleL-Callback");

      while(1){
        if (read(STDIN_FILENO, &ch, 1) == 1)
        {
          if (ch == 'a')
          {
            printf("La touche A a été cliquée !\n");
            if (send(client_socket, &callback_message, sizeof(callback_message), 0) < 0) {
              perror("Erreur lors de l'envoi du message de callback\n");
              return -1;
            }
            break;
          }
        }
      }
      // Restaure les attributs du terminal d'origine
      tcsetattr(STDIN_FILENO, TCSANOW, &originalAttr);
      // Envoyer le message de callback

    }

    if ((strcmp(received_message.message, "pedaleR") == 0 && received_message.mode_jeu == 1)) {
      printf("Le contenu correspond à 'pedaleR' en mode 1 joueur\n");

      // Préparer le message de callback
      struct message callback_message;
      callback_message.mode_jeu = 1;
      callback_message.statut = 1;
      strcpy(callback_message.message, "pedaleR-Callback");
      while(1){
        if (read(STDIN_FILENO, &ch, 1) == 1)
        {
          if (ch == 'b')
          {
            printf("La touche B a été cliquée !\n");
            if (send(client_socket, &callback_message, sizeof(callback_message), 0) < 0) {
              perror("Erreur lors de l'envoi du message de callback\n");
              return -1;
            }
            break;
          }
        }
      }
      // Restaure les attributs du terminal d'origine
      tcsetattr(STDIN_FILENO, TCSANOW, &originalAttr);
      // Envoyer le message de callback
    }

    if (received_message.mode_jeu == 2) {
      printf("Le contenu correspond au mode2Joueurs\n");

      // Préparer le message de callback
      struct message callback_message;
      callback_message.mode_jeu = 2;
      callback_message.statut = 1;
      strcpy(callback_message.message, "partie2Joueurs-Callback");
      while(1){
        if (read(STDIN_FILENO, &ch, 1) == 1)
        {
          if (ch == 'c')
          {
            printf("La touche C a été cliquée !\n");
            if (send(client_socket, &callback_message, sizeof(callback_message), 0) < 0) {
              perror("Erreur lors de l'envoi du message de callback\n");
              return -1;
            }
            break;
          }
        }
      }
      // Restaure les attributs du terminal d'origine
      tcsetattr(STDIN_FILENO, TCSANOW, &originalAttr);
      // Envoyer le message de callback
    }
    // Traitez le message ici

    // Fermez la socket client
    close(client_socket);
  }

  // Fermez le socket du serveur (ce code n'est jamais atteint dans ce cas)
  close(server_socket);

  return 0;
}

void lancement_serveur_socket(void* port){
  intptr_t port_socket_serveur = (intptr_t)port;
  int result = lancer_serveur_socket((int)port_socket_serveur);

  if (result == -1) {
    printf("Erreur lors du démarrage du serveur\n");
    return 1;
  }

}

void lancementPartie2Joueurs(){

}

int main(int argc, char const *argv[]) {
  pthread_t thread,threadPartie2Joueurs;

  int result = pthread_create(&thread, NULL, lancement_serveur_socket, PORT);
  if (result != 0) {
    printf("Erreur lors de la création du thread\n");
    return 1;
  }

  int result2 = pthread_create(&threadPartie2Joueurs, NULL, lancementPartie2Joueurs, PORT);


  printf("Thread créé avec succès\n");

  // Attendre la fin du thread
  result = pthread_join(thread, NULL);
  if (result != 0) {
    printf("Erreur lors de l'attente de la fin du thread\n");
    return 1;
  }
  result2 = pthread_join(thread, NULL);

  printf("Thread terminé\n");

  return 0;
}
