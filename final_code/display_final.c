#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>

#define SHM_KEY 1234

GtkWidget *window;
GtkWidget *labelTextCourse;
GtkWidget *labelTextInter;
GtkWidget *labelTextMeilleur;

GtkWidget *labelChronoCourseGauche;
GtkWidget *labelChronoCourseDroite;
GtkWidget *labelChronoInterGauche;
GtkWidget *labelChronoInterDroite;
GtkWidget *labelChronoInterDroite;
GtkWidget *labelChronoMeilleurGauche;
GtkWidget *labelChronoMeilleurDroite;

gboolean runningLeft = 0;
gboolean runningRight = 0;
int bestTimeLeft = 32767;
int bestTimeRight = 32767;
int lastInterLeft = 0;
int lastInterRight = 0;

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

/**************************************************************************************************/
/*Fonction : openSharedMemory                                                                          */
/* Description :   Nous  créons et attachons une région de mémoire partagée,
renvoyant un pointeur vers la structure de données partagée.                                             */
/**************************************************************************************************/
SharedData* openSharedMemory() {
  int shmid;
  key_t key = SHM_KEY;
  SharedData* sharedData;

  // Nous créons de la mémoire partagée
  if ((shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666)) < 0) {
    perror("Erreur lors de la  Nous créons  de la mémoire partagée");
    exit(1);
  }

  // Nous attachons la mémoire partagée
  if ((sharedData = (SharedData*)shmat(shmid, NULL, 0)) == (SharedData*)-1) {
    perror("Erreur lors de l'attachement de la mémoire partagée");
    exit(1);
  }

  return sharedData;
}

/**************************************************************************************************/
/*Fonction : updateClocks                                                                          */
/* Description :   Nous mettons à jour les affichages des horloges et
des meilleurs temps dans une interface graphique GTK+ en fonction des données partagées.           */
/**************************************************************************************************/
gboolean updateClocks(gpointer data) {
  SharedData* sharedData = (SharedData*) data;

  //Si la pédale gauche est cliquée ou que la partie du joueur gauche est en cours, nous affichons son chrono
  if (runningLeft || sharedData->pedalLeftPressed) {
    runningLeft = TRUE;

    //Nous calculons le temps pour le chrono
    int tempsEcoulePedaleG = sharedData->elapsedMillisLeft;
    int minutesG = (tempsEcoulePedaleG / 60000) % 60;
    int secondsG = (tempsEcoulePedaleG / 1000) % 60;
    int centisecondsG = (tempsEcoulePedaleG / 10) % 100;

    //Nous affichons le chrono
    gchar *timeStrLeft = g_strdup_printf("%02d:%02d:%02d", minutesG, secondsG, centisecondsG);
    gtk_label_set_text(GTK_LABEL(labelChronoCourseGauche), timeStrLeft);
    g_free(timeStrLeft);
  }

    //Si la pédale droite est cliquée ou que la partie du joueur droit est en cours, nous affichons son chrono
  if (runningRight || sharedData->pedalRightPressed) {
    runningRight = TRUE;

    //Nous calculons le temps pour le chrono
    int tempsEcoulePedaleD = sharedData->elapsedMillisRight;
    int minutesD = (tempsEcoulePedaleD / 60000) % 60;
    int secondsD = (tempsEcoulePedaleD / 1000) % 60;
    int centisecondsD = (tempsEcoulePedaleD / 10) % 100;

    //Nous affichons le chrono
    gchar *timeStrRight = g_strdup_printf("%02d:%02d:%02d", minutesD, secondsD, centisecondsD);
    gtk_label_set_text(GTK_LABEL(labelChronoCourseDroite), timeStrRight);
    g_free(timeStrRight);
  }

  //Si le joueur de la pédale gauche a appuyé sur le bouton stop, nous réinitalisons l'affichage à zéro
  if (sharedData->finishedLeft) {
    runningLeft = 0;
    sharedData->finishedLeft = 0;
    sharedData->interLeft = 0;

     //Si le temps en fin de chrono est supérieur au précédent ou non nulle, il devient le meilleur temps
    if (bestTimeLeft != NULL || sharedData->elapsedMillisLeft < bestTimeLeft) {
      bestTimeLeft = sharedData->elapsedMillisLeft;
    }
  }

  //Si le joueur de la pédale droite a appuyé sur le bouton stop, nous réinitalisons l'affichage à zéro
  if (sharedData->finishedRight) {
    runningRight = 0;
    sharedData->finishedRight = 0;
    sharedData->interRight = 0;

    //Si le temps en fin de chrono est supérieur au précédent ou non nulle, il devient le meilleur temps
    if (bestTimeRight != NULL || sharedData->elapsedMillisRight < bestTimeRight) {
      bestTimeRight = sharedData->elapsedMillisRight;
    }
  }

  //Si le nouveau meilleur chrono du joueur gauche est bien en dessous du précédent,  nous l'affichons
  if (bestTimeLeft != 32767) {
    int tempsEcouleMeilleurG = bestTimeLeft;

    int minutesM = (tempsEcouleMeilleurG / 60000) % 60;
    int secondsM = (tempsEcouleMeilleurG / 1000) % 60;
    int centisecondsM = (tempsEcouleMeilleurG / 10) % 100;

    gchar *timeStrBestLeft = g_strdup_printf("%02d:%02d:%02d", minutesM, secondsM, centisecondsM);
    gtk_label_set_text(GTK_LABEL(labelChronoMeilleurGauche), timeStrBestLeft);
    g_free(timeStrBestLeft);
  }

 //Si le nouveau meilleur chrono du joueur droit est bien en dessous du précédent , nous l'affichons
  if (bestTimeRight != 32767) {
    int tempsEcouleMeilleurD = bestTimeRight;

    int minutesM = (tempsEcouleMeilleurD / 60000) % 60;
    int secondsM = (tempsEcouleMeilleurD / 1000) % 60;
    int centisecondsM = (tempsEcouleMeilleurD / 10) % 100;

    gchar *timeStrBestRight = g_strdup_printf("%02d:%02d:%02d", minutesM, secondsM, centisecondsM);
    gtk_label_set_text(GTK_LABEL(labelChronoMeilleurDroite), timeStrBestRight);
    g_free(timeStrBestRight);
  }

  //Si le nouveau meilleur chrono du joueur gauche est bien supérieur du précédent , nous l'affichons
  if (sharedData->interLeft > lastInterLeft) {
    int tempsEcouleInterG = sharedData->interLeft;

    int minutesI = (tempsEcouleInterG / 60000) % 60;
    int secondsI = (tempsEcouleInterG / 1000) % 60;
    int centisecondsI = (tempsEcouleInterG / 10) % 100;

    gchar *timeStrInterLeft = g_strdup_printf("%02d:%02d:%02d", minutesI, secondsI, centisecondsI);
    gtk_label_set_text(GTK_LABEL(labelChronoInterGauche), timeStrInterLeft);
    g_free(timeStrInterLeft);
  }

  //Si le nouveau meilleur chrono du joueur droit est bien supérieur du précédent , nous l'affichons
  if (sharedData->interRight > lastInterRight) {
    int tempsEcouleInterD = sharedData->interRight;

    int minutesI = (tempsEcouleInterD / 60000) % 60;
    int secondsI = (tempsEcouleInterD / 1000) % 60;
    int centisecondsI = (tempsEcouleInterD / 10) % 100;

    gchar *timeStrInterRight = g_strdup_printf("%02d:%02d:%02d", minutesI, secondsI, centisecondsI);
    gtk_label_set_text(GTK_LABEL(labelChronoInterDroite), timeStrInterRight);
    g_free(timeStrInterRight);
  }

  //Si le chrono du joueur gauche, nous mettons à jour à zéro l'affichage
  if (sharedData->elapsedMillisLeft == 0) {
    sharedData->interLeft = 0;
    gchar *timeStrInterLeft = g_strdup_printf("00:00:00");
    gtk_label_set_text(GTK_LABEL(labelChronoInterGauche), timeStrInterLeft);
  }

  //Si le chrono du joueur droite, nous mettons à jour à zéro l'affichage
  if (sharedData->elapsedMillisRight == 0) {
    sharedData->interRight = 0;
    gchar *timeStrInterRight = g_strdup_printf("00:00:00");
    gtk_label_set_text(GTK_LABEL(labelChronoInterDroite), timeStrInterRight);
  }

  return G_SOURCE_CONTINUE;
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  //  Nous créons  de la fenêtre principale
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Climb Race");
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Maximisation de la fenêtre
  gtk_window_maximize(GTK_WINDOW(window));

  //  Nous créons  d'un conteneur parent
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(vbox);

  //  Nous créons  des labels pour les chronomètres
  labelTextCourse = gtk_label_new("Temps de course");
  labelTextInter = gtk_label_new("Temps intermédiaires");
  labelTextMeilleur = gtk_label_new("Meilleurs temps");

  //  Nous créons  du label pour afficher le temps
  PangoFontDescription *fontDesc = pango_font_description_from_string("Monospace 20");

  labelChronoCourseGauche = gtk_label_new("00:00:00");
  gtk_widget_override_font(labelChronoCourseGauche, fontDesc);
  labelChronoCourseDroite = gtk_label_new("00:00:00");
  gtk_widget_override_font(labelChronoCourseDroite, fontDesc);

  labelChronoInterGauche = gtk_label_new("00:00:00");
  gtk_widget_override_font(labelChronoInterGauche, fontDesc);
  labelChronoInterDroite = gtk_label_new("00:00:00");
  gtk_widget_override_font(labelChronoInterDroite, fontDesc);

  labelChronoMeilleurGauche = gtk_label_new("00:00:00");
  gtk_widget_override_font(labelChronoMeilleurGauche, fontDesc);
  labelChronoMeilleurDroite = gtk_label_new("00:00:00");
  gtk_widget_override_font(labelChronoMeilleurDroite, fontDesc);

  // Déclaration des couleurs
  GdkRGBA white;
  GdkRGBA red;
  GdkRGBA blue;
  GdkRGBA yellow;

  //  Nous créons  la première ligne blanche
  GtkWidget *white_case_1 = gtk_frame_new(NULL);
  gtk_widget_set_hexpand(white_case_1, TRUE);
  gdk_rgba_parse(&white, "white");
  gtk_widget_override_background_color(white_case_1, 0, &white);
  gtk_box_pack_start(GTK_BOX(vbox), white_case_1, FALSE, FALSE, 0);
  gtk_widget_set_size_request(white_case_1, -1, 30);
  gtk_widget_show(white_case_1);

  //  Nous créons une hbox pour les cases rouge et bleue section course
  GtkWidget *hboxCourse = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(hboxCourse);
  gtk_box_pack_start(GTK_BOX(vbox), hboxCourse, FALSE, FALSE, 0);
  gtk_widget_set_size_request(hboxCourse, -1, 100);

  //  Nous créons   la case rouge section course
  GtkWidget *red_case_1 = gtk_frame_new(NULL);
  gdk_rgba_parse(&red, "#FF9696");
  gtk_widget_override_background_color(red_case_1, 0, &red);
  gtk_box_pack_start(GTK_BOX(hboxCourse), red_case_1, TRUE, TRUE, 0);
  gtk_widget_show(red_case_1);

  //  Nous créons  de la case bleue section course
  GtkWidget *blue_case_1 = gtk_frame_new(NULL);
  gdk_rgba_parse(&blue, "#9696FF");
  gtk_widget_override_background_color(blue_case_1, 0, &blue);
  gtk_box_pack_start(GTK_BOX(hboxCourse), blue_case_1, TRUE, TRUE, 0);
  gtk_widget_show(blue_case_1);

  //  Nous créons   la deuxième ligne blanche
  GtkWidget *white_case_2 = gtk_frame_new(NULL);
  gtk_widget_set_hexpand(white_case_2, TRUE);
  gdk_rgba_parse(&white, "white");
  gtk_widget_override_background_color(white_case_2, 0, &white);
  gtk_box_pack_start(GTK_BOX(vbox), white_case_2, FALSE, FALSE, 0);
  gtk_widget_set_size_request(white_case_2, -1, 30);
  gtk_widget_show(white_case_2);

  //  Nous créons  une hbox pour les cases rouge et bleue section intermédiaire
  GtkWidget *hboxInter = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(hboxInter);
  gtk_box_pack_start(GTK_BOX(vbox), hboxInter, FALSE, FALSE, 0);
  gtk_widget_set_size_request(hboxInter, -1, 140);

  //  Nous créons   la case blanche gauche section intermédiaire
  GtkWidget *white_case_left = gtk_frame_new(NULL);
  gdk_rgba_parse(&white, "white");
  gtk_widget_override_background_color(white_case_left, 0, &white);
  gtk_box_pack_start(GTK_BOX(hboxInter), white_case_left, TRUE, TRUE, 0);
  gtk_widget_show(white_case_left);

  //  Nous créons   la case blanche droite section intermédiaire
  GtkWidget *white_case_right = gtk_frame_new(NULL);
  gdk_rgba_parse(&white, "white");
  gtk_widget_override_background_color(white_case_right, 0, &white);
  gtk_box_pack_start(GTK_BOX(hboxInter), white_case_right, TRUE, TRUE, 0);
  gtk_widget_show(white_case_right);

  //  Nous créons   la troisième ligne blanche
  GtkWidget *white_case_3 = gtk_frame_new(NULL);
  gtk_widget_set_hexpand(white_case_3, TRUE);
  gdk_rgba_parse(&white, "white");
  gtk_widget_override_background_color(white_case_3, 0, &white);
  gtk_box_pack_start(GTK_BOX(vbox), white_case_3, FALSE, FALSE, 0);
  gtk_widget_set_size_request(white_case_3, -1, 30);
  gtk_widget_show(white_case_3);

  //  Nous créons une hbox pour les cases jaune section meilleurs temps
  GtkWidget *hboxMeilleur = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(hboxMeilleur);
  gtk_box_pack_start(GTK_BOX(vbox), hboxMeilleur, FALSE, FALSE, 0);
  gtk_widget_set_size_request(hboxMeilleur, -1, 100);

  //  Nous créons  la case jaune de gauche
  GtkWidget *yellow_case_1 = gtk_frame_new(NULL);
  gdk_rgba_parse(&yellow, "#FFFF96");
  gtk_widget_override_background_color(yellow_case_1, 0, &yellow);
  gtk_box_pack_start(GTK_BOX(hboxMeilleur), yellow_case_1, TRUE, TRUE, 0);
  gtk_widget_show(yellow_case_1);

  //  Nous créons la case jaune de droite
  GtkWidget *yellow_case_2 = gtk_frame_new(NULL);
  gdk_rgba_parse(&red, "#FFFF96");
  gtk_widget_override_background_color(yellow_case_2, 0, &red);
  gtk_box_pack_start(GTK_BOX(hboxMeilleur), yellow_case_2, TRUE, TRUE, 0);
  gtk_widget_show(yellow_case_2);

  // Nous ajoutons les labels dans les cases
  gtk_container_add(GTK_CONTAINER(white_case_1), labelTextCourse);
  gtk_container_add(GTK_CONTAINER(red_case_1), labelChronoCourseGauche);
  gtk_container_add(GTK_CONTAINER(blue_case_1), labelChronoCourseDroite);
  gtk_container_add(GTK_CONTAINER(white_case_2), labelTextInter);
  gtk_container_add(GTK_CONTAINER(white_case_left), labelChronoInterGauche);
  gtk_container_add(GTK_CONTAINER(white_case_right), labelChronoInterDroite);
  gtk_container_add(GTK_CONTAINER(white_case_3), labelTextMeilleur);
  gtk_container_add(GTK_CONTAINER(yellow_case_1), labelChronoMeilleurGauche);
  gtk_container_add(GTK_CONTAINER(yellow_case_2), labelChronoMeilleurDroite);

  // Nous affichons les labels
  gtk_widget_show(labelTextCourse);
  gtk_widget_show(labelChronoCourseGauche);
  gtk_widget_show(labelChronoCourseDroite);
  gtk_widget_show(labelTextInter);
  gtk_widget_show(labelChronoInterGauche);
  gtk_widget_show(labelChronoInterDroite);
  gtk_widget_show(labelTextMeilleur);
  gtk_widget_show(labelChronoMeilleurGauche);
  gtk_widget_show(labelChronoMeilleurDroite);

  // Nous ajoutons le conteneur parent à la fenêtre
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show(window);

  // Nous initialisons la mémoire partagée une seule fois
  SharedData* sharedData = openSharedMemory();

  // Nous lançons de la mise à jour du chrono
  g_timeout_add(10, updateClocks, sharedData);

  //  Nous lançons  de la boucle principale de GTK
  gtk_main();

  // Nous détachons la mémoire partagée après la fin de la boucle principale
  if (shmdt(sharedData) == -1) {
    perror("Erreur lors du détachement de la mémoire partagée");
    exit(1);
  }

  return 0;
}
