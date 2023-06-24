#include <stdio.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 1234

GtkWidget *window;
GtkWidget *labelClimberLeft;
GtkWidget *labelClimberRight;
GtkWidget *labelChronoLeft;
GtkWidget *labelChronoRight;
gboolean runningLeft = FALSE;
gboolean runningRight = FALSE;

typedef struct {
    int pedalLeftPressed;
	int pedalRightPressed;
    int elapsedMillisLeft;
    int elapsedMillisRight;
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
        gtk_label_set_text(GTK_LABEL(labelChronoLeft), timeStrLeft);
        g_free(timeStrLeft);
    }

    if (runningRight || sharedData->pedalRightPressed) {
        runningRight = TRUE;
        int tempsEcoulePedaleD = sharedData->elapsedMillisRight;

        int minutesD = (tempsEcoulePedaleD / 60000) % 60;
        int secondsD = (tempsEcoulePedaleD / 1000) % 60;
        int centisecondsD = (tempsEcoulePedaleD / 10) % 100;

        gchar *timeStrRight = g_strdup_printf("%02d:%02d:%02d", minutesD, secondsD, centisecondsD);
        gtk_label_set_text(GTK_LABEL(labelChronoRight), timeStrRight);
        g_free(timeStrRight);
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

    // Création des labels pour les chronomètres
    labelClimberLeft = gtk_label_new("Grimpeur de gauche");
    labelClimberRight = gtk_label_new("Grimpeur de droite");

    // Création du label pour afficher le temps
    PangoFontDescription *fontDesc = pango_font_description_from_string("Monospace 30");

    labelChronoLeft = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoLeft, fontDesc);
    labelChronoRight = gtk_label_new("00:00:00");
    gtk_widget_override_font(labelChronoRight, fontDesc);
    
    // Création de la mise en page
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    // Création de deux boîtes de conteneurs pour les colonnes
    GtkWidget *boxLeft = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *boxRight = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    // Création des espacements pour centrer verticalement les labels
    GtkWidget *spacerLeftClimber = gtk_label_new(NULL);
    GtkWidget *spacerLeftChrono = gtk_label_new(NULL);
    GtkWidget *spacerRightClimber = gtk_label_new(NULL);
    GtkWidget *spacerRightChrono = gtk_label_new(NULL);
    
    gtk_widget_set_hexpand(spacerLeftClimber, TRUE);
    gtk_widget_set_hexpand(spacerLeftChrono, TRUE);
    gtk_widget_set_hexpand(spacerRightClimber, TRUE);
    gtk_widget_set_hexpand(spacerRightChrono, TRUE);
    
    // Augmentation de la taille des espacements
    int spacerClimberSize = 60;
    int spacerChronoSize = 40;
    
    gtk_widget_set_size_request(spacerLeftClimber, -1, spacerClimberSize);
    gtk_widget_set_size_request(spacerLeftChrono, -1, spacerChronoSize);
    gtk_widget_set_size_request(spacerRightClimber, -1, spacerClimberSize);
    gtk_widget_set_size_request(spacerRightChrono, -1, spacerChronoSize);
    
    // Ajout des labels des grimpeurs et des chronomètres dans les boîtes de conteneurs
    gtk_box_pack_start(GTK_BOX(boxLeft), spacerLeftClimber, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxLeft), labelClimberLeft, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(boxLeft), spacerLeftChrono, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxLeft), labelChronoLeft, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(boxRight), spacerRightClimber, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxRight), labelClimberRight, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(boxRight), spacerRightChrono, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(boxRight), labelChronoRight, FALSE, FALSE, 0);
    
    // Ajout des boîtes de conteneurs dans la mise en page
    gtk_grid_attach(GTK_GRID(grid), boxLeft, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), boxRight, 1, 0, 1, 1);
    
    // Ajout de la mise en page à la fenêtre
    gtk_container_add(GTK_CONTAINER(window), grid);
    
    // Affichage des éléments
    gtk_widget_show_all(window);

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
