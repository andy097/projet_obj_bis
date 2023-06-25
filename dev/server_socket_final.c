#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define CHECK(sts,msg) if ((sts) == -1 )  { perror(msg); exit(-1);}

struct message {
	int mode_jeu;
	int statut;
	char message[100];
};

void* traiter_client(void* client_socket_ptr) {
	int client_socket = *((int*)client_socket_ptr);
	free(client_socket_ptr);

	struct termios originalAttr, newAttr;
	tcgetattr(STDIN_FILENO, &originalAttr);
	newAttr = originalAttr;
	newAttr.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newAttr);

	char ch;

	// Réception du message structuré
	struct message received_message;
	if (recv(client_socket, &received_message, sizeof(received_message), 0) < 0) {
		perror("Erreur lors de la réception du message\n");
		return NULL;
	}

	if (strcmp(received_message.message, "pedaleG") == 0 && received_message.mode_jeu == 1) {
		printf("Le contenu correspond à 'pedaleG' en mode 1 joueur\n");

		// Préparer le message de callback
		struct message callback_message;
		callback_message.mode_jeu = 1;
		callback_message.statut = 1;
		strcpy(callback_message.message, "pedaleG-Callback");

		while (1) {
			if (read(STDIN_FILENO, &ch, 1) == 1) {
				if (ch == 'a') {
					printf("La touche A a été cliquée !\n");
					if (send(client_socket, &callback_message, sizeof(callback_message), 0) < 0) {
						perror("Erreur lors de l'envoi du message de callback\n");
						break;
					}
					break;
				}
			}
		}

		// Restaure les attributs du terminal d'origine
		tcsetattr(STDIN_FILENO, TCSANOW, &originalAttr);
	}

	if ((strcmp(received_message.message, "pedaleD") == 0 && received_message.mode_jeu == 1)) {
		printf("Le contenu correspond à 'pedaleD' en mode 1 joueur\n");

		// Préparer le message de callback
		struct message callback_message;
		callback_message.mode_jeu = 1;
		callback_message.statut = 1;
		strcpy(callback_message.message, "pedaleD-Callback");
		while(1){
			if (read(STDIN_FILENO, &ch, 1) == 1) {
				if (ch == 'b') {
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

	if ((strcmp(received_message.message, "pedaleG") == 0 && received_message.mode_jeu == 2)) {
		printf("Le contenu correspond à 'pedaleG' en mode 2 joueurs\n");

		while (1) {
			if (read(STDIN_FILENO, &ch, 1) == 1) {
				if (ch == 'c') {
					// Préparer le message de callback
					struct message callback_message;
					callback_message.mode_jeu = 2;
					callback_message.statut = 1;
					strcpy(callback_message.message, "pedaleG-Callback");

					printf("La touche C a été cliquée !\n");
					if (send(client_socket, &callback_message, sizeof(callback_message), 0) < 0) {
						perror("Erreur lors de l'envoi du message de callback\n");
						return -1;
					}
					break;
				}
			}
		}
	}

	if ((strcmp(received_message.message, "pedaleD") == 0 && received_message.mode_jeu == 2)) {
		printf("Le contenu correspond à 'pedaleD' en mode 2 joueurs\n");

		// Préparer le message de callback
		struct message callback_message;
		callback_message.mode_jeu = 2;
		callback_message.statut = 1;
		strcpy(callback_message.message, "pedaleD-Callback");
		while (1) {
			if (read(STDIN_FILENO, &ch, 1) == 1) {
				if (ch == 'd') {
					printf("La touche D a été cliquée !\n");
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

	// Fermez la socket client
	close(client_socket);
	return NULL;
}

void* lancer_serveur_socket(void* port_ptr) {
	intptr_t port = (intptr_t)port_ptr;

	struct sockaddr_in server_address, client_address;
	int server_socket, client_socket, c;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		printf("Erreur lors de la création du socket\n");
		return NULL;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);

	if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
		perror("Erreur lors du bind du socket\n");
		return NULL;
	}

	if (listen(server_socket, MAX_CLIENTS) < 0) {
		perror("Erreur lors de la mise en écoute du socket\n");
		return NULL;
	}

	printf("Serveur en attente de connexions...\n");

	c = sizeof(struct sockaddr_in);

	while (1) {
		client_socket = accept(server_socket, (struct sockaddr*)&client_address, (socklen_t*)&c);
		if (client_socket < 0) {
			perror("Erreur lors de l'acceptation de la connexion\n");
			continue;
		}

		printf("Nouvelle connexion acceptée\n");

		// Créer un thread pour traiter cette connexion
		pthread_t client_thread;
		int* client_socket_ptr = malloc(sizeof(int));
		*client_socket_ptr = client_socket;
		pthread_create(&client_thread, NULL, traiter_client, (void*)client_socket_ptr);

		// Détacher le thread pour qu'il se termine automatiquement
		pthread_detach(client_thread);
	}

	close(server_socket);
	return NULL;
}

int main(int argc, char const* argv[]) {
	pthread_t server_thread;
	intptr_t port = PORT;
	pthread_create(&server_thread, NULL, lancer_serveur_socket, (void*)port);

	// Attendez indéfiniment
	while (1);

	return 0;
}
