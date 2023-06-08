#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *message = "Bonjour, serveur !";
    char buffer[BUFFER_SIZE] = {0};

    // Création du socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Erreur de création du socket\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Conversion de l'adresse IP du serveur de sa représentation textuelle en binaire
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Adresse invalide / Adresse non prise en charge\n");
        return -1;
    }

    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connexion au serveur échouée\n");
        return -1;
    }

    // Envoi du message au serveur
    send(sock, message, strlen(message), 0);
    printf("Message envoyé au serveur\n");

    // Lecture de la réponse du serveur
    valread = read(sock, buffer, BUFFER_SIZE);
    printf("Réponse du serveur : %s\n", buffer);

    return 0;
}

