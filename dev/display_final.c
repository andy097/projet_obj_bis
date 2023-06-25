#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 1234

GtkWidget *window;
GtkWidget *labelTextCourse;
GtkWidget *labelTextInter;
GtkWidget *labelTextMeilleur;

GtkWidget *labelChronoCourseGauche;
GtkWidget *labelChronoCourseDroite;
GtkWidget *labelChronoInterGauche;
GtkWidget *labelChronoInterDroite;
GtkWidget *labelChronoMeilleur;

gboolean runningLeft = FALSE;
gboolean runningRight = FALSE;
int bestTime = 32767;

typedef struct {
    int pedalLeftPressed;
	int pedalRightPressed;
    int elapsedMillisLeft;
    int elapsedMillisRight;
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

gboolean updateClocks(gpointer data) {
    SharedData* sharedData = (SharedData*) data;

    if (runningLeft || sharedData->pedalLeftPressed) {
        runningLeft = TRUE;
        int tempsEcoulePedaleG = sharedData->elapsedMillisLeft;

        int minutesG = (tempsEcoulePedaleG / 60000) % 60;
        int secondsG = (tempsEcoulePedaleG / 1000) % 60;
        int centisecondsG = (tempsEcoulePedaleG / 10) % 100;

        gchar *timeStrLeft = g_strdup_printf("%02d:%02d:%02d", minutesG, secondsG, centisecondsG);
        gtk_label_set_text(GTK_LABEL(labelChronoCourseGauche), timeStrLeft);
        g_free(timeStrLeft);
    }

    if (runningRight || sharedData->pedalRightPressed) {
        runningRight = TRUE;
        int tempsEcoulePedaleD = sharedData->elapsedMillisRight;

        int minutesD = (tempsEcoulePedaleD / 60000) % 60;
        int secondsD = (tempsEcoulePedaleD / 1000) % 60;
        int centisecondsD = (tempsEcoulePedaleD / 10) % 100;

        gchar *timeStrRight = g_strdup_printf("%02d:%02d:%02d", minutesD, secondsD, centisecondsD);
        gtk_label_set_text(GTK_LABEL(labelChronoCourseDroite), timeStrRight);
        g_free(timeStrRight);
    }

    if (sharedData->finishedLeft) {
        runningLeft = FALSE;
        sharedData->finishedLeft = FALSE;

        if (bestTime != NULL || sharedData->elapsedMillisLeft < bestTime) {
            bestTime = sharedData->elapsedMillisLeft;
        }
    }

    if (sharedData->finishedRight) {
        runningRight = FALSE;
        sharedData->finishedRight = FALSE;

        if (bestTime != NULL || sharedData->elapsedMillisRight < bestTime) {
            bestTime = sharedData->elapsedMillisRight;
        }
    }

    if (bestTime != NULL) {
        int tempsEcouleMeilleur = bestTime;

        int minutesM = (tempsEcouleMeilleur / 60000) % 60;
        int secondsM = (tempsEcouleMeilleur / 1000) % 60;
        int centisecondsM = (tempsEcouleMeilleur / 10) % 100;

        gchar *timeStrBest = g_strdup_printf("%02d:%02d:%02d", minutesM, secondsM, centisecondsM);
        gtk_label_set_text(GTK_LABEL(labelChronoMeilleur), timeStrBest);
        g_free(timeStrBest);
    }

    return G_SOURCE_CONTINUE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Création de la fenêtre principale
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Climb Race");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Maximisation de la fenêtre
    gtk_window_maximize(GTK_WINDOW(window));

    // Création d'un conteneur parent
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(vbox);

    // Création des labels pour les chronomètres
    labelTextCourse = gtk_label_new("Temps de course");
    labelTextInter = gtk_label_new("Temps intermédiaires");
    labelTextMeilleur = gtk_label_new("Meilleur temps");

    // Création du label pour afficher le temps
    PangoFontDescription *fontDesc1 = pango_font_description_from_string("Monospace 30");
    PangoFontDescription *fontDesc2 = pango_font_description_from_string("Monospace 20");

    labelChronoCourseGauche = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoCourseGauche, fontDesc1);
    labelChronoCourseDroite = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoCourseDroite, fontDesc1);

    labelChronoInterGauche = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoInterGauche, fontDesc2);
    labelChronoInterDroite = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoInterDroite, fontDesc2);

    labelChronoMeilleur = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoMeilleur, fontDesc2);
    
    // Déclaration des couleurs
    GdkRGBA white;
    GdkRGBA red;
    GdkRGBA blue;
    GdkRGBA yellow;

    // Création de la première ligne blanche
    GtkWidget *white_case_1 = gtk_frame_new(NULL);
    gtk_widget_set_hexpand(white_case_1, TRUE);
    gdk_rgba_parse(&white, "white");
    gtk_widget_override_background_color(white_case_1, 0, &white);
    gtk_box_pack_start(GTK_BOX(vbox), white_case_1, FALSE, FALSE, 0);
    gtk_widget_set_size_request(white_case_1, -1, 40);
    gtk_widget_show(white_case_1);

    // Création d'une hbox pour les cases rouge et bleue section course
    GtkWidget *hboxCourse = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(hboxCourse);
    gtk_box_pack_start(GTK_BOX(vbox), hboxCourse, TRUE, TRUE, 0);
    gtk_widget_set_size_request(hboxCourse, -1, 120);

    // Création de la case rouge section course
    GtkWidget *red_case_1 = gtk_frame_new(NULL);
    gdk_rgba_parse(&red, "#FF9696");
    gtk_widget_override_background_color(red_case_1, 0, &red);
    gtk_box_pack_start(GTK_BOX(hboxCourse), red_case_1, TRUE, TRUE, 0);
    gtk_widget_show(red_case_1);

    // Création de la case bleue section course
    GtkWidget *blue_case_1 = gtk_frame_new(NULL);
    gdk_rgba_parse(&blue, "#9696FF");
    gtk_widget_override_background_color(blue_case_1, 0, &blue);
    gtk_box_pack_start(GTK_BOX(hboxCourse), blue_case_1, TRUE, TRUE, 0);
    gtk_widget_show(blue_case_1);

    // Création de la deuxième ligne blanche
    GtkWidget *white_case_2 = gtk_frame_new(NULL);
    gtk_widget_set_hexpand(white_case_2, TRUE);
    gdk_rgba_parse(&white, "white");
    gtk_widget_override_background_color(white_case_2, 0, &white);
    gtk_box_pack_start(GTK_BOX(vbox), white_case_2, FALSE, FALSE, 0);
    gtk_widget_set_size_request(white_case_2, -1, 40);
    gtk_widget_show(white_case_2);

    // Création d'une hbox pour les cases rouge et bleue section intermédiaire
    GtkWidget *hboxInter = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(hboxInter);
    gtk_box_pack_start(GTK_BOX(vbox), hboxInter, TRUE, TRUE, 0);

    // Création de la case rouge section intermédiaire
    GtkWidget *red_case_2 = gtk_frame_new(NULL);
    gdk_rgba_parse(&red, "#FF9696");
    gtk_widget_override_background_color(red_case_2, 0, &red);
    gtk_box_pack_start(GTK_BOX(hboxInter), red_case_2, TRUE, TRUE, 0);
    gtk_widget_show(red_case_2);

    // Création de la case bleue section intermédiaire
    GtkWidget *blue_case_2 = gtk_frame_new(NULL);
    gdk_rgba_parse(&blue, "#9696FF");
    gtk_widget_override_background_color(blue_case_2, 0, &blue);
    gtk_box_pack_start(GTK_BOX(hboxInter), blue_case_2, TRUE, TRUE, 0);
    gtk_widget_show(blue_case_2);

    // Création de la troisième ligne blanche
    GtkWidget *white_case_3 = gtk_frame_new(NULL);
    gtk_widget_set_hexpand(white_case_3, TRUE);
    gdk_rgba_parse(&white, "white");
    gtk_widget_override_background_color(white_case_3, 0, &white);
    gtk_box_pack_start(GTK_BOX(vbox), white_case_3, FALSE, FALSE, 0);
    gtk_widget_set_size_request(white_case_3, -1, 40);
    gtk_widget_show(white_case_3);

    // Création d'une case jaune qui prend toute la largeur de la fenêtre
    GtkWidget *yellow_case = gtk_frame_new(NULL);
    gtk_widget_set_hexpand(yellow_case, TRUE);
    gdk_rgba_parse(&yellow, "#FFFF96");
    gtk_widget_override_background_color(yellow_case, 0, &yellow);
    gtk_box_pack_start(GTK_BOX(vbox), yellow_case, FALSE, FALSE, 0);
    gtk_widget_set_size_request(yellow_case, -1, 80);
    gtk_widget_show(yellow_case);

    // Ajout des labels dans les cases
    gtk_container_add(GTK_CONTAINER(white_case_1), labelTextCourse);
    gtk_container_add(GTK_CONTAINER(red_case_1), labelChronoCourseGauche);
    gtk_container_add(GTK_CONTAINER(blue_case_1), labelChronoCourseDroite);
    gtk_container_add(GTK_CONTAINER(white_case_2), labelTextInter);
    gtk_container_add(GTK_CONTAINER(red_case_2), labelChronoInterGauche);
    gtk_container_add(GTK_CONTAINER(blue_case_2), labelChronoInterDroite);
    gtk_container_add(GTK_CONTAINER(white_case_3), labelTextMeilleur);
    gtk_container_add(GTK_CONTAINER(yellow_case), labelChronoMeilleur);

    // Affichage des labels
    gtk_widget_show(labelTextCourse);
    gtk_widget_show(labelChronoCourseGauche);
    gtk_widget_show(labelChronoCourseDroite);
    gtk_widget_show(labelTextInter);
    gtk_widget_show(labelChronoInterGauche);
    gtk_widget_show(labelChronoInterDroite);
    gtk_widget_show(labelTextMeilleur);
    gtk_widget_show(labelChronoMeilleur);

    // Ajout du conteneur parent à la fenêtre
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(window);

    // Initialisez la mémoire partagée une seule fois
    SharedData* sharedData = openSharedMemory();
    
    // Lancement de la mise à jour du chrono
    g_timeout_add(10, updateClocks, sharedData);
    
    // Lancement de la boucle principale de GTK
    gtk_main();

    // Détachez la mémoire partagée après la fin de la boucle principale
    if (shmdt(sharedData) == -1) {
        perror("Erreur lors du détachement de la mémoire partagée");
        exit(1);
    }
    
    return 0;
}
