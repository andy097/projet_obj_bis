#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *response = "Message reçu !";

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Échec de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Attacher le socket à un port spécifique
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Échec de l'attachement du socket au port");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Attachement du socket au port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Échec de l'attachement du socket au port");
        exit(EXIT_FAILURE);
    }

    // Écoute des connexions entrantes
    if (listen(server_fd, 3) < 0) {
        perror("Erreur lors de l'écoute des connexions");
        exit(EXIT_FAILURE);
    }

    // Accepter les connexions entrantes
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Erreur lors de l'acceptation de la connexion");
        exit(EXIT_FAILURE);
    }

    // Lire le message envoyé par le client
    read(new_socket, buffer, BUFFER_SIZE);
    printf("Message du client : %s\n", buffer);

    // Envoyer une réponse au client
    send(new_socket, response, strlen(response), 0);
    printf("Réponse envoyée au client\n");

    return 0;
}

