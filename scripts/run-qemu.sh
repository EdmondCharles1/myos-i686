#!/bin/bash
# run-qemu.sh - Lance le kernel dans QEMU

KERNEL_BIN="../kernel/build/myos.bin"

# Vérifier que le kernel existe
if [ ! -f "$KERNEL_BIN" ]; then
    echo "❌ Erreur: kernel non trouvé à $KERNEL_BIN"
    echo "Compilez d'abord avec 'make' depuis la racine du projet"
    exit 1
fi

echo "🚀 Lancement de myos-i686 dans QEMU..."
echo ""

# Lancer QEMU
qemu-system-i386 \
    -kernel "$KERNEL_BIN" \
    -m 128M \
    -display gtk

# Alternative sans interface graphique :
# qemu-system-i386 -kernel "$KERNEL_BIN" -nographic
