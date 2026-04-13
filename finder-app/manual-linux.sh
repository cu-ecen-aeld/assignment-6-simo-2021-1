#!/bin/bash
# ==============================================================================
# Script d'installation et de compilation manuel du noyau Linux ARM64 + rootfs/BusyBox
# Auteur original : Siddhant Jajoo
# Modifications/Adaptations : Arnaud (18DEC2025)
# Objectif : Compiler un noyau Linux pour QEMU ARM64 et préparer un rootfs fonctionnel
# ==============================================================================
# Summrize of steps
# 	- Download source for common packages from upstream
#	- Cross Copilation
#	- Build Components
#	- Assemble rootfs in staging area
#	- Create image files
# ==============================================================================

# ------------------------------------------------------------------------------
# CONFIGURATION GLOBALE (adaptée à l'environnement de l'utilisateur)
# ------------------------------------------------------------------------------
set -e  # Arrêter le script en cas d'erreur
set -u  # Arrêter le script si une variable non définie est utilisée
# set -x  # Décommenter pour debug (affiche toutes les commandes)

# Chemins principaux (ABSOLUS pour éviter les erreurs de chemin relatif)
	OUTDIR=/home/tchuinkou/aeld          # Dossier de sortie principal
	# i-1) git clone the linux kernel
	KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
	KERNEL_VERSION=v5.15.163             # Version du noyau Linux
	BUSYBOX_VERSION=1_33_1               # Version de BusyBox
	FINDER_APP_DIR=$(realpath $(dirname $0))  # Dossier du script finder-app
	ARCH=arm64                           # Architecture cible
	#aarch64-none-linux-gnu-  et aarch64-linux-gnu-  sont identique mais chez moi  j ai installé:aarch64-linux-gnu- 
	CROSS_COMPILE=aarch64-linux-gnu- # Cross-compilateur ARM64

# Variables spécifiques au rootfs (dossiers critiques)
	DEV_DIR="${OUTDIR}/rootfs/dev"       # Dossier /dev du rootfs (périphériques)
	ROOTFS_BIN="${OUTDIR}/rootfs/bin"    # Dossier /bin du rootfs (binaires BusyBox)
	WRITER_DIR="/home/tchuinkou/aesd-assignments/finder-app"  # Dossier du writer.c/Makefile

	#read -p "Appuie sur Entrée pour continuer..."
	#echo -e "\n"

# ------------------------------------------------------------------------------
echo "=== GESTION DU DOSSIER DE SORTIE (OUTDIR) ==="
# ------------------------------------------------------------------------------
	if [ $# -ge 1 ]; then
	    OUTDIR=$1
	    echo "[1/17] Utilisation du dossier passé en argument : ${OUTDIR}"
	else
	    echo "[1/17] Utilisation du dossier par défaut : ${OUTDIR}"
	fi

	# Créer le dossier de sortie si il n'existe pas
	mkdir -p ${OUTDIR}

	#read -p "Appuie sur Entrée pour continuer..."
	#echo -e "\n"

# ------------------------------------------------------------------------------
echo "=== ÉTAPE 1 : CLONAGE DU DÉPÔT LINUX-STABLE (si non présent) ==="
# ------------------------------------------------------------------------------
	cd "$OUTDIR"
	if [ ! -d "${OUTDIR}/linux-stable" ]; then
	    echo "[2/17] CLONAGE DU DÉPÔT LINUX STABLE (version ${KERNEL_VERSION})"
	    git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
	fi

	#read -p "Appuie sur Entrée pour continuer..."
	#echo -e "\n"

# ------------------------------------------------------------------------------
echo "=== ÉTAPE 2 : COMPILATION DU NOYAU LINUX ARM64 ==="
# ------------------------------------------------------------------------------
# Compiler le noyau uniquement si Image n'existe pas
if [ ! -e "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" ]; then
    cd linux-stable
    
    # Étape 2.1 : Checkout + nettoyage
    echo "[3/17] Checkout de la version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    echo -e "\n[2.1/17] Nettoyage des anciennes compilations..."
    make ARCH=${ARCH} clean -j$(nproc)
    
# TODO: Add your kernel build steps here
    # Étape 2.2 : Configuration + compilation (noyau + modules)
    #read -p "Appuyez sur ENTER pour ouvrir menuconfig (sauvegardez la config)..."
    echo -e "\n[2.2/17] Compilation noyau + modules ARM64..."
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig menuconfig oldconfig Image modules
    read -p "Appuyez sur ENTER pour continuer..."
    # Étape 2.3 : Installation des modules + vérifications
    echo -e "\n[2.3/17] Installation des modules dans le rootfs..."
    mkdir -p ${OUTDIR}/rootfs
    sudo make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${OUTDIR}/rootfs modules_install

    # Vérifications critiques (arrêt en cas d'échec)
    [ ! -f "arch/${ARCH}/boot/Image" ] && { echo "ERREUR : Image non généré !"; exit 1; }
    [ ! -d "${OUTDIR}/rootfs/lib/modules" ] && { echo "ERREUR : Modules non installés !"; exit 1; }

    # Étape 2.4 : Compilation DTB QEMU (virt.dtb)
    echo -e "\n[2.8/17] Compilation du Device Tree..."
    grep -q "CONFIG_ARCH_VIRT=y" .config || { echo "CONFIG_ARCH_VIRT=y" >> .config; make ARCH=${ARCH} olddefconfig -j$(nproc); }
    make ARCH=${ARCH} dtbs -j$(nproc)

    # Vérification DTB
    VIRT_DTB_PATH="arch/${ARCH}/boot/dts/virt/virt.dtb"
    [ ! -f "${VIRT_DTB_PATH}" ] && { echo "ERREUR : virt.dtb non généré ! (${VIRT_DTB_PATH})"; exit 1; }

    # Étape 2.5 : Copie des fichiers + vérification finale
    echo -e "\n[2.9/17] Copie des fichiers dans ${OUTDIR}..."
    cp arch/${ARCH}/boot/Image ${OUTDIR}/
    cp ${VIRT_DTB_PATH} ${OUTDIR}/qemu-virt.dtb

    # Résumé final
    echo -e "\n Compilation terminée ! Vérification :"
    ls -lh ${OUTDIR}/Image ${OUTDIR}/qemu-virt.dtb ${OUTDIR}/rootfs/lib/modules/
    #read -p "Appuyez sur ENTER pour continuer..."
# END TODO
fi

#read -p "Appuie sur Entrée pour continuer..."
echo -e "\n"

# ------------------------------------------------------------------------------
echo "=== ÉTAPE 3 : PRÉPARATION DU ROOTFS (système de fichiers racine) ==="
# ------------------------------------------------------------------------------
echo "[4/17] Adding the Image in outdir"
echo "[5/17] Creating the staging directory for the root filesystem"

	# Supprimer le rootfs existant (réinitialisation complète)
	cd "$OUTDIR"
	if [ -d "${OUTDIR}/rootfs" ]; then
	    echo "[6/17] Deleting rootfs directory at ${OUTDIR}/rootfs and starting over " #Suppression du dossier rootfs existant (réinitialisation)..."
	    sudo rm -rf ${OUTDIR}/rootfs
	fi
	
# TODO: Create necessary base directories
# Création de la structure de dossiers obligatoire pour Linux
echo "[7/17] Création du dossier rootfs principal..."
	mkdir -p "${OUTDIR}/rootfs"

	echo "[8/17] Création des dossiers système obligatoires..."
	mkdir -p "${OUTDIR}/rootfs"/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var}
	mkdir -p "${OUTDIR}/rootfs"/{usr/bin,usr/sbin,usr/lib}  # Sous-dossiers /usr
	mkdir -p "${OUTDIR}/rootfs/var/log"                     # Dossier logs

#read -p "Appuie sur Entrée pour continuer..."
echo -e "\n"
# END TODO

# TODO: Make and install busybox
# ------------------------------------------------------------------------------
echo "=== ÉTAPE 4 : COMPILATION ET INSTALLATION DE BUSYBOX ==="
# ------------------------------------------------------------------------------
	cd "$OUTDIR"
	if [ ! -d "${OUTDIR}/busybox" ]; then
	    echo "[9/17] CLONAGE DU DÉPÔT BUSYBOX (version ${BUSYBOX_VERSION})..."
	    git clone git://busybox.net/busybox.git
	    cd busybox
	    git checkout ${BUSYBOX_VERSION}
	else
	    cd busybox
	fi

	# Configuration de BusyBox (defconfig si pas de config existante)
	echo "[10/17] Configuration de BusyBox..."
	if [ ! -f .config ]; then 
	    make defconfig  # Créer la config par défaut SEULEMENT si elle n'existe pas
	fi

	# Forcer la compilation statique de BusyBox (pas de dépendances de librairies)
	sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config

	# Compilation de BusyBox (cross-compilation ARM64)
	echo "[11/17] Compilation de BusyBox (statique)..."
	make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

	# Installation de BusyBox dans le rootfs
	echo "[12/17] Installation de BusyBox dans ${OUTDIR}/rootfs..."
	make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

	read -p "Appuie sur Entrée pour continuer..."  
	echo "                                                                                            "
	# Vérification des dépendances de bibliothèques BusyBox (statique = pas de dépendances)
	echo "[13/17] Vérification des dépendances de BusyBox (statique)..."
	${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep -E "program interpreter|Shared library" || echo " BusyBox est statique (pas de dépendances)"

#read -p "Appuie sur Entrée pour continuer..."  
echo -e "\n"
# END TODO

echo "Library dependencies"
#${CROSS_COMPILE}readelf -a ${OUTDIR}/busybox | grep "program interpreter"
#${CROSS_COMPILE}readelf -a ${OUTDIR}/busybox | grep "Shared library"
#read -p "Appuie sur Entrée pour continuer..." 


# TODO: Make device nodes
# ------------------------------------------------------------------------------
echo "=== ÉTAPE 5 : CRÉATION DES PÉRIPHÉRIQUES (/dev) ==="
# ------------------------------------------------------------------------------
	# Les device nodes permettent au noyau/BusyBox de communiquer avec le matériel virtuel (QEMU)
	echo "[14/17] Création des device nodes dans ${DEV_DIR}..."
	mkdir -p "${DEV_DIR}"  # S'assurer que le dossier existe

	# Création des périphériques essentiels (mode 666 = lecture/écriture pour tous)
	sudo mknod -m 666 "${DEV_DIR}/console" c 5 1   # Console système
	sudo mknod -m 666 "${DEV_DIR}/null"    c 1 3   # Périphérique null
	sudo mknod -m 666 "${DEV_DIR}/tty"     c 5 0   # Terminal interactif
	sudo mknod -m 666 "${DEV_DIR}/zero"    c 1 5   # Générateur d'octets nuls
	sudo mknod -m 666 "${DEV_DIR}/random"  c 1 8   # Générateur de nombres aléatoires
	sudo mknod -m 666 "${DEV_DIR}/urandom" c 1 9   # Générateur aléatoire non bloquant

#read -p "Appuie sur Entrée pour continuer..."  
echo -e "\n"
# END TODO


# TODO: Clean and build the writer utility
# ------------------------------------------------------------------------------
echo "=== ÉTAPE 6 : COMPILATION DE L'UTILITAIRE \"writer\" ==="
# ------------------------------------------------------------------------------
echo "[15/17] Compilation de l'utilitaire writer..."

	cd "${WRITER_DIR}" 
	pwd
	echo ""
	read -p "2-Appuie sur Entrée pour continuer..."  
	make clean 2>/dev/null  # Nettoyage des anciennes compilations (sans afficher erreurs)
	 
	aarch64-linux-gnu-gcc -o writer writer.c -Wall -g   
	
	file writer
	
	#make CROSS_COMPILE=${CROSS_COMPILE} all -j$(nproc)     # Compilation ARM64
	 
	# Vérification : Le fichier writer doit exister
	if [ ! -f "${WRITER_DIR}/writer" ]; then
	    echo "ERREUR : Échec de la compilation du writer !"
	    read -p "Appuie sur Entrée pour continuer..."  
	    exit 1
	fi
# END TODO


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
# ------------------------------------------------------------------------------
# ÉTAPE 7 : COPIE DES SCRIPTS FINDER DANS LE ROOTFS
# ------------------------------------------------------------------------------
echo "[16/17] Copie des scripts/executables finder dans /home du rootfs..."
	mkdir -p "${OUTDIR}/rootfs/home"  # S'assurer que le dossier existe
	cp "${WRITER_DIR}/finder.sh"        		${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/finder-test.sh"   		${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/writer.sh"        		${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/writer"           		${OUTDIR}/rootfs/home/
	#cp "${WRITER_DIR}/conf/assignment.txt"  	${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/manual-linux.sh"  		${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/start-qemu-app.sh" 		${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/start-qemu-terminal.sh" 	${OUTDIR}/rootfs/home/
	cp "${WRITER_DIR}/autorun-qemu.sh"  		${OUTDIR}/rootfs/home/
	cp -rL "${WRITER_DIR}/conf"  			${OUTDIR}/rootfs/home/

# Rendre les scripts exécutables
	chmod +x ${OUTDIR}/rootfs/home/*.sh
	chmod +x ${OUTDIR}/rootfs/home/writer  # Binaire writer (ESSENTIEL)
# END TODO




# 2. Compiler /init en binaire statique ARM64
	#${CROSS_COMPILE}gcc -static -O2 -o /tmp/init /tmp/init.c

# 3. Copier /init dans le rootfs et définir les droits
	#sudo cp /tmp/init ${OUTDIR}/rootfs/init
	#sudo chown root:root ${OUTDIR}/rootfs/init
	#sudo chmod 755 ${OUTDIR}/rootfs/init  # rwxr-xr-x (droits root standard)

# Vérification de /init
	echo " Fichier /init (binaire statique) créé :"
	#ls -lh ${OUTDIR}/rootfs/init
	#${CROSS_COMPILE}file ${OUTDIR}/rootfs/init | grep "statically linked" && echo " /init est statique"

# TODO: Chown the root directory
# Attribution des droits root (obligatoire pour le rootfs Linux)
	echo "[17/17] Attribution des permissions root au rootfs..."
	sudo chown -R root:root ${OUTDIR}/rootfs/
# END TODO

# TODO: Create initramfs.cpio.gz
# Création de l'initramfs.cpio.gz (format newc OBLIGATOIRE)
	echo "[FINAL] Création de l'initramfs.cpio.gz..."
	#read -p "Appuie sur Entrée pour continuer..."  

# Vérifier que le rootfs existe
	if [ ! -d "${OUTDIR}/rootfs" ]; then
	    echo "ERREUR : Dossier rootfs introuvable à ${OUTDIR}/rootfs"
	    exit 1
	fi

# Générer le cpio (format newc, droits root, sans warnings)
	cd "${OUTDIR}/rootfs" || { echo "ERREUR : Impossible d'accéder à ${OUTDIR}/rootfs"; exit 1; }
	sudo find . -print0 | cpio --null -ov --format=newc --owner root:root 2>/dev/null > "${OUTDIR}/initramfs.cpio"

# Compresser en gz
	cd "${OUTDIR}" || exit 1
	gzip -9 -f initramfs.cpio

	# Vérification finale
	if [ -f "${OUTDIR}/initramfs.cpio.gz" ]; then
	    echo -e "\n initramfs.cpio.gz créé avec succès :"
	    ls -lh "${OUTDIR}/initramfs.cpio.gz"
	else
	    echo " Échec de la création de initramfs.cpio.gz"
	    exit 1
	fi
# END TODO

# ------------------------------------------------------------------------------
# FIN DU SCRIPT
# ------------------------------------------------------------------------------
	echo -e "\nSCRIPT TERMINÉ ! Tous les fichiers sont prêts dans ${OUTDIR} :"
	echo "   - Noyau : ${OUTDIR}/Image"
	echo "   - Device Tree : ${OUTDIR}/qemu-virt.dtb"
	echo "   - Initramfs : ${OUTDIR}/initramfs.cpio.gz"
	echo "   - Rootfs : ${OUTDIR}/rootfs"

