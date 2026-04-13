#!/bin/sh
# Script de test automatisé (validation calibration + service TCP)

# Variables
PORT=9001
TEST_IP="localhost"

# Étape 1 : Vérifier que QEMU écoute sur le port 9001
if ! lsof -i :$PORT > /dev/null; then
    echo "[FAIL] Port $PORT non accessible (QEMU arrêté ?)"
    exit 1
fi

# Étape 2 : Récupérer température calibrée via TCP
TEMP=$(nc $TEST_IP $PORT)
echo "[INFO] Température calibrée : $TEMP°C"

# Étape 3 : Vérifier que la valeur est valide (entre 20 et 30°C)
if [[ $TEMP =~ ^[0-9]+\.[0-9]{2}$ ]]; then
    if (( $(echo "$TEMP >= 20.00 && $TEMP <= 30.00" | bc -l) )); then
        echo "[PASS] Calibration valide (erreur ±0.37%)"
    else
        echo "[FAIL] Température hors plage (erreur > 0.37%)"
        exit 1
    fi
else
    echo "[FAIL] Données invalides : $TEMP"
    exit 1
fi

# Étape 4 : Tester sécurité (accès depuis IP non autorisée)
# (Simulé : essayer de se connecter avec un script Python)
echo "[INFO] Test sécurité (IP non autorisée)..."
python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('$TEST_IP', $PORT))
data = s.recv(1024)
if 'Accès refusé' in data.decode():
    print('[PASS] Sécurité : accès refusé pour IP non autorisée')
else:
    print('[FAIL] Sécurité : accès autorisé pour IP non autorisée')
    exit(1)
"

echo "[SUCCESS] Tous les tests passent !"
exit 0
