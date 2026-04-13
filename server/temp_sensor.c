#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Calibration parameters (ton résultat : ±0.37% error)
#define CALIB_FACTOR 0.987
#define CALIB_OFFSET 1.24

// Simule une valeur brute du capteur (erreur ±8%)
float get_raw_temperature() {
    // Initialiser le générateur aléatoire
    srand(time(NULL));
    // Température réelle (entre 20°C et 30°C)
    float real_temp = 20 + (rand() % 10);
    // Ajouter erreur ±8%
    float error = (rand() % 16) - 8; // -8 à +8
    float raw_temp = real_temp + (real_temp * error / 100);
    return raw_temp;
}

// Applique la calibration
float calibrate_temperature(float raw_temp) {
    return (raw_temp * CALIB_FACTOR) + CALIB_OFFSET;
}

// NEW!!!!!!!!!!!!!!!!!!
// Envoyer la température calibrée sur le bus CAN (vcan0)
void send_temp_over_can(float calibrated_temp) {
    // Convertir la température en entier (ex: 24.56°C → 2456)
    int temp_int = (int)(calibrated_temp * 100);
    
    // Commande CAN : envoyer sur vcan0, ID 0x123, données [temp_high, temp_low]
    char can_cmd[100];
    snprintf(can_cmd, sizeof(can_cmd), 
             "cansend vcan0 123#%02X%02X", 
             (temp_int >> 8) & 0xFF,  // Octet haut
             temp_int & 0xFF);        // Octet bas
    
    // Exécuter la commande CAN (simule envoi UDS)
    system(can_cmd);
    printf("Données CAN envoyées : ID=0x123, Temp=%d (0x%04X)\n", temp_int, temp_int);
}

int main() {
    // Test de la calibration
    float raw = get_raw_temperature();
    float calibrated = calibrate_temperature(raw);
    
    printf("=== Capteur Température ===\n");
    printf("Valeur brute : %.2f°C (erreur ±8%%)\n", raw);
    printf("Valeur calibrée : %.2f°C (erreur ±0.37%%)\n", calibrated);
    
    // NEW!!!
     // Envoyer sur CAN
    send_temp_over_can(calibrated);
    
    // Sauvegarder dans un fichier pour le service TCP
    FILE *f = fopen("/var/tmp/temp_data", "w");
    if (f == NULL) {
        perror("Erreur écriture fichier");
        return 1;
    }
    fprintf(f, "%.2f", calibrated);
    fclose(f);
    
    return 0;
}
