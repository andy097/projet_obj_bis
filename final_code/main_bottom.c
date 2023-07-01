#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <wiringPi.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define CHECK(sts,msg) if ((sts) == -1 ) { perror(msg); exit(-1);}

#define PEDALG_PIN 17 // PIN de la pedale L
#define PEDALD_PIN 22 // PIN de la pedale R
#define BUZZER_PIN 18 // PIN du buzzer

#define SHM_KEY 1234 // Clé de la mémoire partagée
#define DELAI_APPUI 3000 // Délai minimale de 3 secondes
#define SERVER_IP "192.168.19.98" // Adresse IP du serveur installé sur le RPI_MINI

pthread_mutex_t mutexPedaleG, mutexPedaleD, mutexlancementPartie, mutexMode2Joueurs;
pthread_cond_t conditionLancementPartie = PTHREAD_COND_INITIALIZER;
pthread_cond_t conditionLancementSocket = PTHREAD_COND_INITIALIZER;

int joueurGPret = 0, joueurDPret = 0;
int phaseInitialisationEnCours = 0, partieEnCours = 0;
int ret = 0;
int controle_break = 0;
int mode_2_joueurs = 0;
int mode_1_joueur = 0, indice_pedale_mode_1_joueur = 0;
int joueurGParti = 0, joueurDParti = 0;
int controleLancerPartie = 0;
int faux_depart = 0;
int enceinte_active = 1;
int premieractiveG = 0;
int premieractiveD = 0;

//Variables pour les informations temporelles
double tempsActuelG = 0,  tempsActuelD = 0; // Temps actuel en millisecondes
int tempsEcoulePedaleG = 0, tempsEcoulePedaleD = 0;
int minutesG, secondsG, centisecondsG;
int minutesD, secondsD, centisecondsD;

// Socket
int controleLancementSocket = 0;
int indiceMessageAEnvoye = 0;
pthread_mutex_t mutexLancementSocket;
pthread_cond_t conditionLancementSocket;

//Gestion des appuis court et long
int appuiCourtActivePedaleG = 0;
int appuiCourtActivePedaleD = 0;
int appuiLongActivePedaleG = 0;
int appuiLongActivePedaleD = 0;

// Structure du message à envoyer au socket serveur
struct message {
	int mode_jeu; // mode de jeu : 1 ou 2 joueurs
	int statut; // partie commencé ou terminé
	char message[100]; //Nom de la pédale concerné
};

typedef struct {
	int pedalLeftPressed;
	int pedalRightPressed;
	int elapsedMillisLeft;
	int elapsedMillisRight;
	int interLeft;
	int interRight;
	int finishedLeft;
	int finishedRight;
} SharedData;

SharedData* openSharedMemory() {
	int shmid;
	key_t key = SHM_KEY;
	SharedData* sharedData;

	// Création de la mémoire partagée
	if ((shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666)) < 0) {
		perror("Erreur lors de la création de la mémoire partagée");
		exit(1);
	}

	// Attacher la mémoire partagée
	if ((sharedData = (SharedData*)shmat(shmid, NULL, 0)) == (SharedData*)-1) {
		perror("Erreur lors de l'attachement de la mémoire partagée");
		exit(1);
	}

	return sharedData;
}

/**************************************************************************************************/
/*Fonction : emettreSon                                                                          */
/* Description :   Nous générons un son avec une fréquence et une durée spécifiées en utilisant un buzzer */
/**************************************************************************************************/
void emettreSon(int frequency, int duration) {
	int halfPeriod = 1000000 / frequency; // Nous calculons la demi-période en microsecondes
	int iterations = duration * 1000 / halfPeriod; // Nous calculons le nombre d'itérations pour la durée spécifiée

	for (int i = 0; i < iterations; i++) {
		digitalWrite(BUZZER_PIN, HIGH);
		delayMicroseconds(halfPeriod / 2);

		digitalWrite(BUZZER_PIN, LOW);
		delayMicroseconds(halfPeriod / 2);
	}
}

/**************************************************************************************************/
/*Fonction : gererPedaleG                                                                          */
/* Description :   Nous gérons les évènements se produisant sur la pédale gauche										*/
/**************************************************************************************************/
void* gererPedaleG(void* arg) {
	double tempsDepart = 0; // Temps de départ en millisecondes

	SharedData* sharedData = openSharedMemory();

	// Initialisation des données partagées
	sharedData->pedalLeftPressed = 0;
	sharedData->pedalRightPressed = 0;
	sharedData->elapsedMillisLeft = 0;
	sharedData->elapsedMillisRight = 0;
	sharedData->interLeft = 0;
	sharedData->interRight = 0;
	sharedData->finishedLeft = 0;
	sharedData->finishedRight = 0;

	while (1) {

		//Nous récupérons l'état de la pédale G sur son port GPIO
		int pedalState = digitalRead(PEDALG_PIN);

		// Si la pédaleG n'est pas encore appuyée et que son état à l'état haut
		if (!sharedData->pedalLeftPressed && pedalState == HIGH) {
			pthread_mutex_lock(&mutexPedaleG);
			// La pédale gauche est maintenue appuyée
			sharedData->pedalLeftPressed = 1;
			printf("Grimpeur G - Prêts ?\n");
			// Nous mémorisons le temps de départ
			tempsDepart = millis();
			pthread_mutex_unlock(&mutexPedaleG);
		}

		//Si la pédaleG est maintenue appuyé et que la partie d'escalade n'a pas encore commencé
		if (sharedData->pedalLeftPressed && pedalState == HIGH && !partieEnCours) {
			pthread_mutex_lock(&mutexPedaleG);
			//Nous récupérons le temps actuel
			tempsActuelG = millis();
			//Nous calculons le délai d'appui de la pédale gauche
			double elapsedMillis = tempsActuelG - tempsDepart;
			pthread_mutex_unlock(&mutexPedaleG);
			// Si le délai d'appui est supérieur à 3 secondes, le joueurG est prêt pour la phase d'initialisation d'une partie d'escalade.
			if (elapsedMillis >= DELAI_APPUI) {
				pthread_mutex_lock(&mutexPedaleG);
				joueurGPret = 1;
				pthread_mutex_unlock(&mutexPedaleG);
			}
		}

		// Si la pédaleG est relachée, nous réinistalisons le temps de départ
		if (sharedData->pedalLeftPressed && pedalState == LOW && !partieEnCours) {
			pthread_mutex_lock(&mutexPedaleG);
			tempsDepart = 0; // Nous réinitialisons le temps de départ
			sharedData->pedalLeftPressed = 0; // La pédale a été relâchée
			joueurGPret = 0; // Le joueur est considéré comme non prêt pour la phase d'initialisation d'une partie d'escalade.
			pthread_mutex_unlock(&mutexPedaleG);
			printf("Grimpeur G - pédale G relaché !\n");
			//Nous émettons le son du faux départ
			if (faux_depart && enceinte_active) {
				emettreSon(200, 300);
			}
		}

		//Si le joueur n'est plus prêt et que nous sommes dans le cas mode d'un joueur, nous réinialisons la phase d'initialisation d'une partie d'escalade.
		if (!joueurGPret && mode_1_joueur && indice_pedale_mode_1_joueur == 1) {
			pthread_mutex_lock(&mutexPedaleG);
			printf("Grimpeur G - Non Prêt - Pedale G !\n");
			mode_1_joueur = 0;
			indice_pedale_mode_1_joueur = 0;
			joueurGPret = 0;
			controle_break = 1;
			pthread_mutex_unlock(&mutexPedaleG);
		}

		//Si les 2 joueurs sont prêts, nous actions le mode 2 joueurs
		if (joueurGPret && joueurDPret) {
			pthread_mutex_lock(&mutexPedaleG);
			mode_2_joueurs = 1;
			mode_1_joueur = 0;
			indice_pedale_mode_1_joueur = 0;
			pthread_mutex_unlock(&mutexPedaleG);
		}

		//Si nous sommes dans le mode 2 joueurs et qu'un des 2 joueurs n'est plus prêt, nous réinitialisatons la phase d'initialisation d'une partie d'escalade.
		if (mode_2_joueurs && (!joueurGPret || !joueurDPret)) {
			pthread_mutex_lock(&mutexPedaleG);
			printf("Grimpeur G - mode 2 joueurs - Non Prêt \n" );
			joueurGPret = 0;
			joueurDPret = 0;
			mode_2_joueurs = 0;
			controle_break = 1;
			pthread_mutex_unlock(&mutexPedaleG);
		}
	}
	// Nous détachons la mémoire partagée
	if (shmdt(sharedData) == -1) {
		perror("Erreur lors du détachement de la mémoire partagée");
	}
}


/**************************************************************************************************/
/*Fonction : gererPedaleD                                                                          */
/* Description :   Nous gérons les évènements se produisant sur la pédale droite									*/
/**************************************************************************************************/
void* gererPedaleD(void* arg) {

	double tempsDepart = 0; // Temps de départ en millisecondes
	SharedData* sharedData = openSharedMemory();

	// Initialisation des données partagées
	sharedData->pedalLeftPressed = 0;
	sharedData->pedalRightPressed = 0;
	sharedData->elapsedMillisLeft = 0;
	sharedData->elapsedMillisRight = 0;
	sharedData->interLeft = 0;
	sharedData->interRight = 0;
	sharedData->finishedLeft = 0;
	sharedData->finishedRight = 0;

	while (1) {

		//Nous récupérons l'état de la pédale D sur son port GPIO
		int pedalState = digitalRead(PEDALD_PIN);

		// Si la pédaleD n'est pas encore appuyée et que son état à l'état haut
		if (!sharedData->pedalRightPressed && pedalState == HIGH) {
			pthread_mutex_lock(&mutexPedaleD);
			// La pédale droite est maintenue appuyée
			sharedData->pedalRightPressed = 1;
			printf("Grimpeur D - Prêt ?\n");
			// Nous mémorisons le temps de départ
			tempsDepart = millis();
			pthread_mutex_unlock(&mutexPedaleD);
		}

		//Si la pédaleG est maintenue appuyé et que la partie du jeu n'a pas encore commencé
		if (sharedData->pedalRightPressed && pedalState == HIGH && !partieEnCours) {
			pthread_mutex_lock(&mutexPedaleD);
			tempsActuelD = millis();
			pthread_mutex_unlock(&mutexPedaleD);
			//Nous calculons le délai d'appui de la pédale gauche
			double elapsedMillis = tempsActuelD - tempsDepart;
			// Si le délai d'appui est supérieur à 3 secondes, le joueurD est prêt pour la phase d'initialisation d'une partie d'escalade.
			if (elapsedMillis >= DELAI_APPUI) {
				pthread_mutex_lock(&mutexPedaleD);
				joueurDPret = 1;
				pthread_mutex_unlock(&mutexPedaleD);
			}
		}

		// Si la pédale D est relâchée, nous réinitialisons le temps de départ
		if (sharedData->pedalRightPressed && pedalState == LOW && !partieEnCours) {
			pthread_mutex_lock(&mutexPedaleD);
			tempsDepart = 0; // Nous réinitialisons le temps de départ
			sharedData->pedalRightPressed = 0; // La pédale a été relâchée
			joueurDPret = 0; // Le joueur est considéré comme non prêt pour la phase d'initialisation d'une partie d'escalade.
			pthread_mutex_unlock(&mutexPedaleD);
			printf("Grimpeur D - pédale D relâchée !\n");
			//Nous émettons le son du faux départ
			if (faux_depart && enceinte_active) {
				emettreSon(200, 300);
			}
		}

		//Si nous sommes dans le mode 2 joueurs et qu'un des 2 joueurs n'est plus prêt, nous réinitialisatons la phase d'initialisation d'une partie d'escalade.
		if (!joueurDPret && mode_1_joueur && indice_pedale_mode_1_joueur == 2) {
			pthread_mutex_lock(&mutexPedaleD);
			printf("Grimpeur D - Non Prêt - Pedale D !\n");
			mode_1_joueur = 0;
			indice_pedale_mode_1_joueur = 0;
			joueurDPret = 0;
			controle_break = 1;
			pthread_mutex_unlock(&mutexPedaleD);
		}

		//Si les 2 joueurs sont prêts, nous actions le mode 2 joueurs
		if (joueurGPret && joueurDPret) {
			pthread_mutex_lock(&mutexPedaleD);
			mode_2_joueurs = 1;
			mode_1_joueur = 0;
			indice_pedale_mode_1_joueur = 0;
			pthread_mutex_unlock(&mutexPedaleD);
		}

		//Si nous sommes dans le mode 2 joueurs et qu'un des 2 joueurs n'est plus prêt, nous réinitialisatons la phase d'initialisation d'une partie d'escalade.
		if (mode_2_joueurs && (!joueurGPret || !joueurDPret)) {
			pthread_mutex_lock(&mutexPedaleD);
			printf("Grimpeur D - mode 2 joueurs - Non Prêt \n" );
			joueurGPret = 0;
			joueurDPret = 0;
			mode_2_joueurs = 0;
			controle_break = 1;
			pthread_mutex_unlock(&mutexPedaleD);
		}
	}
	// Détacher la mémoire partagée
	if (shmdt(sharedData) == -1) {
		perror("Erreur lors du détachement de la mémoire partagée");
	}
}


/**************************************************************************************************/
/*Fonction : gererAppuis                                                                          */
/* Description :   Nous gérons les évènements des appuis et longs. Ils permettent	respectivement
d'enregistrer le chrono intermédiaire du joueur, et d'arrêter la partie en cours et réinitialiser le système						*/
/**************************************************************************************************/
void* gererAppuis(void* arg) {
	double tempsDepart = 0; // Temps de départ en millisecondes
	int pedaleGAppuyee = 0; //Variable pour indiquer si la pédaleG est appuyée
	int pedaleDAppuyee = 0; //Variable pour indiquer si la pédaleD est appuyée

	while (1) {

		//Nous récupérons les états de la pédale G sur son port GPIO
		int pedalStateG = digitalRead(PEDALG_PIN);
		int pedalStateD = digitalRead(PEDALD_PIN);


		// Si la pédaleG n'est pas encore appuyée et que son état à l'état haut
		if (!pedaleGAppuyee && pedalStateG == HIGH && partieEnCours) {
			pthread_mutex_lock(&mutexPedaleG);
			pedaleGAppuyee = 1; // La pédale est maintenue appuyée
			printf("GERER APPUI GAUCHE\n");
			// Nous mémorisons le temps de départ
			tempsDepart = millis();
			pthread_mutex_unlock(&mutexPedaleG);
		}

		// Si la pédale G est relâchée et que la partie en cours, nous réinitialisons le temps de départ
		if (pedaleGAppuyee && pedalStateG == LOW && partieEnCours) {
			pthread_mutex_lock(&mutexPedaleG);
			tempsDepart = 0; //  Nous réinitialisons le temps de départ
			pedaleGAppuyee = 0; // La pédale a été relâchée
			joueurGPret = 0; // La pédale appuyée après délai est invalide
			pthread_mutex_unlock(&mutexPedaleG);
			printf("Grimpeur G - pédale G relâchée !\n");
		}

		//Si la pédaleG est maintenue appuyé et que la partie du jeu a commencé
		if (pedaleGAppuyee && pedalStateG == HIGH  && partieEnCours) {
			double elapsedMillis = millis() - tempsDepart;
			if (elapsedMillis <= 2000 && elapsedMillis >= 1000) {
				pthread_mutex_lock(&mutexPedaleG);
				appuiLongActivePedaleG = 1;
				premieractiveG = 1;
				pthread_mutex_unlock(&mutexPedaleG);
			} else if ((elapsedMillis < 1000) && (elapsedMillis >= 500)) {
				pthread_mutex_lock(&mutexPedaleG);
				appuiCourtActivePedaleG = 1;
				premieractiveG = 1;
				pthread_mutex_unlock(&mutexPedaleG);
			}
		}

		// Si la pédaleD n'est pas encore appuyée et que son état à l'état haut
		if (!pedaleDAppuyee && pedalStateD == HIGH && partieEnCours) {
			pthread_mutex_lock(&mutexPedaleD);
			pedaleDAppuyee = 1; // La pédale est maintenue appuyée
			// Nous mémorisons le temps de départ
			tempsDepart = millis();
			pthread_mutex_unlock(&mutexPedaleD);
		}

		// Si la pédale D est relâchée et que la partie en cours, nous réinitialisons le temps de départ
		if (pedaleDAppuyee && pedalStateD == LOW && partieEnCours) {
			pthread_mutex_lock(&mutexPedaleD);
			tempsDepart = 0; // Nous réinitialisons le temps de départ
			pedaleDAppuyee = 0; // La pédale a été relâchée
			joueurDPret = 0; // La pédale appuyée après délai est invalide
			pthread_mutex_unlock(&mutexPedaleD);
			printf("Grimpeur D - pédale D relâchée !\n");
		}

		//Si la pédaleD est maintenue appuyé et que la partie du jeu a commencé
		if (pedaleDAppuyee && pedalStateD == HIGH  && partieEnCours) {
			double elapsedMillis = millis() - tempsDepart;
			if (elapsedMillis <= 2000 && elapsedMillis >= 1000) {
				pthread_mutex_lock(&mutexPedaleD);
				appuiLongActivePedaleD = 1;
				premieractiveD = 1;
				pthread_mutex_unlock(&mutexPedaleD);
			} else if ((elapsedMillis < 1000) && (elapsedMillis >= 500)) {
				pthread_mutex_lock(&mutexPedaleD);
				appuiCourtActivePedaleD = 1;
				premieractiveD = 1;
				pthread_mutex_unlock(&mutexPedaleD);
			}
		}
	}
}

/**************************************************************************************************/
/*Fonction : demarrerJeu                                                                          */
/* Description :   Nous gérons la phase d'initialisation avant le début d'une partie. Dans cette
phase d'initialisation, nous synchronisons les pédales des 2 joueurs si ils sont prêts en utilisant une phase d'émission de sons             */
/**************************************************************************************************/
void* demarrerJeu(void* arg) {
	faux_depart = 1;
	while (1) {

		//Si un des 2 joueurs est prêt, nous la phase d'initialisation avant une partie d'escalade.
		if (joueurGPret || joueurDPret) {
			controle_break = 0;

			//Si joueurG est prêt, nous activons le mode 1 joueur
			if (joueurGPret) {
				pthread_mutex_lock(&mutexPedaleG);
				printf("Grimpeur G - Prêt - Pédale G !\n");
				mode_1_joueur = 1;
				indice_pedale_mode_1_joueur = 1;
				pthread_mutex_unlock(&mutexPedaleG);
			}

			//Si joueurD est prêt, nous activons le mode 1 joueur
			if (joueurDPret) {
				pthread_mutex_lock(&mutexPedaleD);
				printf("Grimpeur D  - Prêt - Pédale D !\n");
				mode_1_joueur = 1;
				indice_pedale_mode_1_joueur = 2;
				pthread_mutex_unlock(&mutexPedaleD);
			}

			//Si les 2 joueurs sont prêts, nous activons le mode 2 joueurs
			if (joueurGPret && joueurDPret) {
				mode_2_joueurs = 1;
			}

			//Nous émettons 3 sonneries. Durant cette émission, nous vérifions qu'aucun joueur n'a laché sa pédale
			for (int i = 0; i < 3; i++) {
				printf("i %d\n", i);

				if (i < 2) {
					if (enceinte_active) {
						emettreSon(300, 300);
					}
				}
				delay(2000);

				//Si un joueur n'est plus prêt, nous sortons de la boucle d'émission servant de phase durant laquelle nous vérifions
			//	que les joueurs sont synchros. Nous réinialisons la phase d'initialisation d'une partie d'escalade
				if (controle_break) {
					phaseInitialisationEnCours = 0;
					break;
				}
				phaseInitialisationEnCours = 1;
			}

			//En sortie de boucle après le 3ème son, nous mettons à un les variables indiquant que les joueurs sont partis. Nous réveillons le socket qui lance la partie.
			if (phaseInitialisationEnCours) {
				pthread_mutex_lock(&mutexlancementPartie);
				controleLancerPartie = 1;
				joueurGParti = 1;
				joueurDParti = 1;
				pthread_cond_signal(&conditionLancementPartie);
				pthread_mutex_unlock(&mutexlancementPartie);
			}
		}
	}
}

/**************************************************************************************************/
/*Fonction : lancer_client_socket                                                                          */
/* Description :   Nous établissons une connexion cliente pour communiquer avec le serveur tournant sur le RPI TOP.
Pour cela, nous lançons un socket client en fonction du mode de jeu (1 ou 2 joueurs) et des pédales concernés.
Nous attendons les messages du serveur pour arreter la partie d'escalade en cours en fonction des cas de modes et des pédales                                                                                                 */
/**************************************************************************************************/
int lancer_client_socket(int port, struct message *msg) {
	int retour_socket_client_socket_client = -1;
	int client_socket;
	struct sockaddr_in addresse_serveur;

	SharedData* sharedData = openSharedMemory();

	// Initialisation des données partagées
	sharedData->pedalLeftPressed = 0;
	sharedData->pedalRightPressed = 0;
	sharedData->elapsedMillisLeft = 0;
	sharedData->elapsedMillisRight = 0;
	sharedData->interLeft = 0;
	sharedData->interRight = 0;
	sharedData->finishedLeft = 0;
	sharedData->finishedRight = 0;

	// Nous créons le socket client
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == -1) {
		printf("Erreur lors de la création du socket\n");
	}

	// Nous préparons l'adresse IPV4 pour le serveur avec lequel nous allons établir la connexion cliente
	addresse_serveur.sin_family = AF_INET;
	addresse_serveur.sin_addr.s_addr = inet_addr(SERVER_IP);
	addresse_serveur.sin_port = htons(port);

	// Nous établissons une connexion client avec le serveur
	if (connect(client_socket, (struct sockaddr*)&addresse_serveur, sizeof(addresse_serveur)) < 0) {
		perror("Erreur lors de la connexion au serveur\n");
		retour_socket_client = 1;
	}

	// La connexion du client au serveur a réussi
	printf("Connecté au serveur\n");

	// Nous envoyons le message au serveur
	if (send(client_socket, msg, sizeof(struct message), 0) < 0) {
		perror("Erreur lors de l'envoi du message\n");
		retour_socket_client = 1;
	}

	printf("Message envoyé avec succès\n");

	// Nous attendons l'acquittement du serveur
	struct message response;
	ssize_t received_bytes = recv(client_socket, &response, sizeof(struct message), 0);
	if (received_bytes < 0) {
		perror("Erreur lors de la réception de la réponse du serveur\n");
	} else if (received_bytes == 0) {
		printf("La connexion avec le serveur a été fermée\n");
		retour_socket_client = 1;
	}

	// Nous fermons le socket client
	if (close(client_socket) == -1) {
		perror("Erreur lors de la fermeture du socket client");
	} else {
		printf("Socket client fermé avec succès\n");

		// Nous castons du pointeur du message reçu vers la structure appropriée
		struct message* message_recu = (struct message*)&response;

		//Si nous recevons un message du serveur pour la pédaleG en mode 1 joueur, nous arretons la partie du joueurG
		if (message_recu->mode_jeu == 1 && (strcmp(message_recu->message, "pedaleG-Callback") == 0)) {
			pthread_mutex_lock(&mutexPedaleG);
			joueurGPret = 0;
			partieEnCours = 0;
			printf("1 Mode Joueur - pedaleG-Callback\n");
			printf("mode jeu %d\n",message_recu->mode_jeu );
			sharedData->finishedLeft = 1;
			pthread_mutex_unlock(&mutexPedaleG);
		}

		//Si nous recevons un message du serveur pour la pédaleD en mode 1 joueur, nous arretons la partie du joueurG
		if (message_recu->mode_jeu == 1 && (strcmp(message_recu->message, "pedaleD-Callback") == 0)) {
			pthread_mutex_lock(&mutexPedaleD);
			joueurDPret = 0;
			partieEnCours = 0;
			printf("1 Mode Joueur - pedaleD-Callback\n");
			sharedData->finishedRight = 1;
			pthread_mutex_unlock(&mutexPedaleD);
		}

		//Si nous recevons un message du serveur pour la pédaleG en mode 2 joueurs, nous arretons la partie du joueurG
		if (message_recu->mode_jeu == 2 && (strcmp(message_recu->message, "pedaleG-Callback") == 0)) {
			pthread_mutex_lock(&mutexMode2Joueurs);
			joueurGPret = 0;
			joueurGParti = 0;
			//Si le chrono de la deuxime pédale (pédaleD) est terminé, nous quittons la partie d'escalade en cours.
			if (!joueurDParti) {
				partieEnCours = 0;
			}
			printf("Mode 2 Joueurs - pedaleG-Callback\n");
			sharedData->finishedLeft = 1;
			pthread_mutex_unlock(&mutexMode2Joueurs);
		}

		//Si nous recevons un message du serveur pour la pédaleD en mode 2 joueurs, nous arretons la partie du joueurG
		if (message_recu->mode_jeu == 2 && (strcmp(message_recu->message, "pedaleD-Callback") == 0)) {
			pthread_mutex_lock(&mutexMode2Joueurs);
			joueurDPret = 0;
			joueurDParti = 0;
			//Si le chrono de la deuxime pédale (pédaleG) est terminé, nous quittons la partie d'escalade en cours.
			if (!joueurGParti) {
				partieEnCours = 0;
			}
			printf("Mode 2 Joueurs - pedaleD-Callback\n");
			sharedData->finishedRight = 1;
			pthread_mutex_unlock(&mutexMode2Joueurs);
		}
	}
	// Nous détachons la mémoire partagée
	if (shmdt(sharedData) == -1) {
		perror("Erreur lors du détachement de la mémoire partagée");
	}
	return retour_socket_client;
}


/**************************************************************************************************/
/*Fonction : lancerSocketClient                                                                          */
/* Description :  Nous gérons le lancement du socket client en fonction du mode de jeu et								*/
/* des pédales concernées, en envoyant des messages au serveur et en attendant les réponses 						  */
/*pour arrêter la partie en cours si nécessaire.  Cette fonction est utilisée par le thread_socket_client */
/**************************************************************************************************/
void* lancerSocketClient(void* arg) {
	while (1) {
		// Nous créons un message à envoyer au serveur
		pthread_mutex_lock(&mutexLancementSocket);
		int lancement_SocketPedaleGMode1Joueur = 0, lancement_SocketPedaleDMode1Joueur = 0, lancement_SocketPedaleGMode2Joueurs = 0, lancement_SocketPedaleDMode2Joueurs = 0;

		struct message msg_PedaleGMode1Joueur;
		msg_PedaleGMode1Joueur.mode_jeu = 1;
		msg_PedaleGMode1Joueur.statut = 0;
		strcpy(msg_PedaleGMode1Joueur.message, "pedaleG");

		struct message msg_PedaleDMode1Joueur;
		msg_PedaleDMode1Joueur.mode_jeu = 1;
		msg_PedaleDMode1Joueur.statut = 0;
		strcpy(msg_PedaleDMode1Joueur.message, "pedaleD");

		struct message msg_PedaleGMode2Joueurs;
		msg_PedaleGMode2Joueurs.mode_jeu = 2;
		msg_PedaleGMode2Joueurs.statut = 0;
		strcpy(msg_PedaleGMode2Joueurs.message, "pedaleG");

		struct message msg_PedaleDMode2Joueurs;
		msg_PedaleDMode2Joueurs.mode_jeu = 2;
		msg_PedaleDMode2Joueurs.statut = 0;
		strcpy(msg_PedaleDMode2Joueurs.message, "pedaleD");

		int port = 8080;

		//Nous mettons en attente le thread tant qu'il n'a pas reçu le signal de condition de réveil
		while (!controleLancementSocket) {
			pthread_cond_wait(&conditionLancementSocket, &mutexLancementSocket);
		}

		controleLancementSocket = 0;
		//En fonction de l'indice du message, nous envoyns le contenu approprié au serveur pour chaque mode (1 ou 2 joueur) et pédales (G ou/et D)
		//Nous appellons la fonction lancer_client_socket en spécifiant le port et le message à envoyer au serveur
		switch (indiceMessageAEnvoye) {
			case 1:
			lancement_SocketPedaleGMode1Joueur = lancer_client_socket(port, &msg_PedaleGMode1Joueur);
			break;
			case 2:
			lancement_SocketPedaleDMode1Joueur = lancer_client_socket(port, &msg_PedaleDMode1Joueur);
			break;
			case 3:

			lancement_SocketPedaleGMode2Joueurs = lancer_client_socket(port, &msg_PedaleGMode2Joueurs);
			lancement_SocketPedaleDMode2Joueurs = lancer_client_socket(port, &msg_PedaleDMode2Joueurs);
			break;
		}
		indiceMessageAEnvoye = 0;
		pthread_mutex_unlock(&mutexLancementSocket);
	}
}

void* lancerPartie(void* arg) {
	while (1) {
		pthread_mutex_lock(&mutexlancementPartie);

		//Nous mettons en attente le thread tant qu'il n'a pas reçu le signal de condition de réveil
		while (!controleLancerPartie) {
			pthread_cond_wait(&conditionLancementPartie, &mutexlancementPartie);
		}

		partieEnCours = 1;
		controleLancerPartie = 0;
		pthread_mutex_unlock(&mutexlancementPartie);

		const int indice_pedale_mode_1_joueur_stockee = indice_pedale_mode_1_joueur;
		const int mode_2_joueurs_stockee = mode_2_joueurs;
		SharedData* sharedData = openSharedMemory();

		// Nous initialisons des données partagées
		sharedData->pedalLeftPressed = 0;
		sharedData->pedalRightPressed = 0;
		sharedData->elapsedMillisLeft = 0;
		sharedData->elapsedMillisRight = 0;
		sharedData->interLeft = 0;
		sharedData->interRight = 0;
		sharedData->finishedLeft = 0;
		sharedData->finishedRight = 0;

		faux_depart = 0;
		enceinte_active = 0;
		//Nous émettons le son pour indiquer que la partie commence
		emettreSon(500, 300);
		//Nous sommes dans la partie en cours
		while (partieEnCours) {

			//Si le joueurG en mode 1 joueur, nous lançons son chrono
			if (indice_pedale_mode_1_joueur_stockee == 1 && joueurGParti) {
				indiceMessageAEnvoye = 1;
				controleLancementSocket = 1;
				// Nous réveillons le thread_socket_client en déclenchant le signal de la condition
				pthread_cond_signal(&conditionLancementSocket);
				tempsEcoulePedaleG = millis() - tempsActuelG;
				sharedData->elapsedMillisLeft = tempsEcoulePedaleG;

				minutesG = (tempsEcoulePedaleG / 60000) % 60;
				secondsG = (tempsEcoulePedaleG / 1000) % 60;
				centisecondsG = (tempsEcoulePedaleG / 10) % 100;
				printf("Temps écoulé - Pedale G : %02d:%02d:%02d\n", minutesG, secondsG, centisecondsG);
				delay(5);

				//Si l'appui court est actif, nous enregistrons le chrono intermédiare du joueurD
				if (appuiCourtActivePedaleG && premieractiveG) {
					sharedData->interLeft = tempsEcoulePedaleG;
					delay(1000);
					appuiCourtActivePedaleG = 0;
					premieractiveG = 0;
				}

				//Si l'appui long est actif, nous réinitialisons notre système à l'état initial
				if (appuiLongActivePedaleG && premieractiveG) {
					delay(3000);
					partieEnCours = 0;
					joueurGParti = 0;
					joueurDParti = 0;
					appuiLongActivePedaleG = 0;
					premieractiveG = 0;
				}
			}

			//Si le joueurD en mode 1 joueur, nous lançons son chrono
			if (indice_pedale_mode_1_joueur_stockee == 2 && joueurDParti) {
				indiceMessageAEnvoye = 2;
				controleLancementSocket = 1;
				// Nous réveillons le thread_socket_client en déclenchant le signal de la condition
				pthread_cond_signal(&conditionLancementSocket);
				tempsEcoulePedaleD = millis() - tempsActuelD;
				sharedData->elapsedMillisRight = tempsEcoulePedaleD;

				minutesD = (tempsEcoulePedaleD / 60000) % 60;
				secondsD = (tempsEcoulePedaleD / 1000) % 60;
				centisecondsD = (tempsEcoulePedaleD / 10) % 100;
				printf("Temps écoulé - Pedale D : %02d:%02d:%02d\n", minutesD, secondsD, centisecondsD);

				//Si l'appui court est actif, nous enregistrons le chrono intermédiare du joueurG
				if (appuiCourtActivePedaleD && premieractiveD) {
					sharedData->interRight = tempsEcoulePedaleD;
					delay(1000);
					appuiCourtActivePedaleD = 0;
					premieractiveD = 0;
				}

				//Si l'appui long est actif, nous réinitialisons notre système à l'état initial
				if (appuiLongActivePedaleD && premieractiveD) {
					delay(3000);
					partieEnCours = 0;
					joueurGParti = 0;
					joueurDParti = 0;
					premieractiveD = 0;
				}
			}

			//Si le mode 2 joueurs activée
			if (mode_2_joueurs_stockee) {
				indiceMessageAEnvoye = 3;
				controleLancementSocket = 1;
				// Nous réveillons le thread_socket_client en déclenchant le signal de la condition
				pthread_cond_signal(&conditionLancementSocket);
				//Si le joueurG est parti, nous lançons son chrono
				if (joueurGParti) {
					tempsEcoulePedaleG = millis() - tempsActuelG;
					sharedData->elapsedMillisLeft = tempsEcoulePedaleG;

					minutesG = (tempsEcoulePedaleG / 60000) % 60;
					secondsG = (tempsEcoulePedaleG / 1000) % 60;
					centisecondsG = (tempsEcoulePedaleG / 10) % 100;
					printf("Temps écoulé - Pedale G : %02d:%02d:%02d\n", minutesG, secondsG, centisecondsG);

					//Si l'appui court est actif, nous enregistrons le chrono intermédiare du joueurG
					if (appuiCourtActivePedaleG && premieractiveG) {
						sharedData->interLeft = tempsEcoulePedaleG;
						delay(1000);
						appuiCourtActivePedaleG = 0;
						premieractiveG = 0;
					}

					//Si l'appui long est actif, nous réinitialisons notre système à l'état initial
					if (appuiLongActivePedaleG && premieractiveG) {
						delay(3000);
						partieEnCours = 0;
						joueurGParti = 0;
						joueurDParti = 0;
						premieractiveG = 0;
					}
				}

				//Si le joueurD est parti, nous lançons son chrono
				if (joueurDParti) {
					tempsEcoulePedaleD = millis() - tempsActuelD;
					sharedData->elapsedMillisRight = tempsEcoulePedaleD;

					minutesD = (tempsEcoulePedaleD / 60000) % 60;
					secondsD = (tempsEcoulePedaleD / 1000) % 60;
					centisecondsD = (tempsEcoulePedaleD / 10) % 100;
					printf("Temps écoulé - Pedale D : %02d:%02d:%02d\n", minutesD, secondsD, centisecondsD);

					//Si l'appui court est actif, nous enregistrons le chrono intermédiare du joueurD
					if (appuiCourtActivePedaleD && premieractiveD) {
						sharedData->interRight = tempsEcoulePedaleD;
						delay(1000);
						appuiCourtActivePedaleD = 0;
						premieractiveD = 0;
					}

					//Si l'appui long est actif, nous réinitialisons notre système à l'état initial
					if (appuiLongActivePedaleD && premieractiveD) {
						delay(3000);
						partieEnCours = 0;
						joueurGParti = 0;
						joueurDParti = 0;
						premieractiveD = 0;
					}
				}
			}
		}

		enceinte_active = 1;
		// Nous détachons la mémoire partagée
		if (shmdt(sharedData) == -1) {
			perror("Erreur lors du détachement de la mémoire partagée");
		}
	}
}

int main() {
	pthread_t thread_pedaleG, thread_pedaleD, thread_initialisation_partie, thread_socket_client, thread_execution_partie, thread_appuis;

	CHECK(wiringPiSetupGpio(),"Erreur lors de l'initialisation de WiringPi\n");
	pinMode(BUZZER_PIN, OUTPUT); // Nous configurons la broche en mode sortie
	pinMode(PEDALG_PIN, INPUT); // Nous configurons la broche en mode entrée
	pinMode(PEDALD_PIN, INPUT);

	//Nous initialisons les mutex nécessaires pour verrouiller les variables partagées
	pthread_mutex_init(&mutexPedaleG, NULL);
	pthread_mutex_init(&mutexPedaleD, NULL);
	pthread_mutex_init(&mutexlancementPartie, NULL);
	pthread_mutex_init(&mutexMode2Joueurs, NULL);

	//Nous créons les threads qui permettent de paralléliser les tâches
	pthread_create(&thread_pedaleG, NULL, gererPedaleG, NULL);
	pthread_create(&thread_pedaleD, NULL, gererPedaleD, NULL);
	pthread_create(&thread_initialisation_partie, NULL, demarrerJeu, NULL);
	pthread_create(&thread_socket_client, NULL, lancerSocketClient, NULL);
	pthread_create(&thread_execution_partie, NULL, lancerPartie, NULL);
	pthread_create(&thread_appuis, NULL, gererAppuis, NULL);

	while(1);

	pthread_join(thread_pedaleG, NULL);
	pthread_join(thread_pedaleD, NULL);
	pthread_join(thread_initialisation_partie, NULL);
	pthread_join(thread_socket_client, NULL);
	pthread_join(thread_appuis, NULL);

	pthread_mutex_destroy(&mutexPedaleG);
	pthread_mutex_destroy(&mutexPedaleD);
	pthread_mutex_destroy(&mutexlancementPartie);
	pthread_mutex_destroy(&mutexMode2Joueurs);
}
