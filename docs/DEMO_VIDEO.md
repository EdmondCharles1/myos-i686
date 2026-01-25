# Script de Démonstration Vidéo - myos-i686

## Configuration de capture

### Options QEMU recommandées
```bash
qemu-system-i386 -kernel kernel/build/myos.bin \
    -m 32M \
    -vga std \
    -display sdl,window_close=off
```

### Résolution et lisibilité
- Résolution : 640x400 (VGA standard)
- Couleur de fond : noir
- Texte : blanc/vert clair
- Police : monospace (par défaut QEMU)

### Outils de capture recommandés
- OBS Studio (Linux/Windows/Mac)
- SimpleScreenRecorder (Linux)
- QEMU built-in : `-display vnc=:1` + client VNC

---

## SCÉNARIO DE DÉMONSTRATION (8-10 minutes)

### PARTIE 1 : Introduction (1 min)

**[00:00-00:30] Écran titre**
```
(Montrer le boot du système)
Attendre que le shell apparaisse...
```

**[00:30-01:00] Présentation**
```
help
```
*Narration : "myos-i686 est un mini-OS éducatif implémentant un noyau
x86 32 bits avec gestion de processus, 6 algorithmes d'ordonnancement,
IPC, synchronisation et gestion mémoire."*

---

### PARTIE 2 : Informations système (1 min)

**[01:00-01:30]**
```
info
uptime
```
*Narration : "Le système affiche les informations de configuration :
architecture, timer à 100Hz, et l'ordonnanceur actif."*

---

### PARTIE 3 : Gestion des processus (2 min)

**[01:30-02:00] Création de processus**
```
ps
spawn test1
spawn test2
spawn test3
ps
```
*Narration : "Nous créons trois processus de test. La commande ps
affiche leur état : READY, RUNNING, etc."*

**[02:00-02:30] États des processus**
```
states
```
*Narration : "Cette commande démontre les transitions d'états :
NEW -> READY -> RUNNING -> BLOCKED -> TERMINATED."*

**[02:30-03:30] Blocage/déblocage**
```
ps
block 2
ps
unblock 2
ps
```
*Narration : "On peut bloquer un processus pour simuler une attente I/O,
puis le débloquer pour le remettre en file READY."*

---

### PARTIE 4 : Ordonnancement (2-3 min)

**[03:30-04:00] Algorithmes disponibles**
```
sched
```
*Narration : "myos supporte 6 algorithmes d'ordonnancement."*

**[04:00-04:30] Démonstration FCFS**
```
sched fcfs
demo
queue
```
*Narration : "FCFS - First Come First Served : les processus s'exécutent
dans l'ordre d'arrivée, sans préemption."*

**[04:30-05:00] Démonstration Round Robin**
```
sched rr
queue
log
```
*Narration : "Round Robin : chaque processus reçoit un quantum de temps
(10 ticks = 100ms), puis passe au suivant."*

**[05:00-05:30] Démonstration Priority**
```
sched priority
bench 3
queue
```
*Narration : "Ordonnancement par priorité : les processus haute priorité
passent en premier."*

**[05:30-06:00] Démonstration avancée**
```
sched mlfq
bench 5
queue
log
```
*Narration : "MLFQ - Multi-Level Feedback Queue : 3 niveaux avec
quantum croissant pour optimiser la réactivité."*

---

### PARTIE 5 : IPC - Communication inter-processus (1 min)

**[06:00-06:30]**
```
mbox create test
mbox send 1 "Hello World"
mbox recv 1
mbox list
```
*Narration : "Le système IPC permet la communication entre processus
via des mailboxes et des messages."*

---

### PARTIE 6 : Synchronisation (1 min)

**[06:30-07:00] Mutex**
```
mutex create verrou
mutex lock 1
mutex list
mutex unlock 1
```
*Narration : "Les mutex permettent l'exclusion mutuelle pour protéger
les ressources partagées."*

**[07:00-07:30] Sémaphores**
```
sem create compteur 5
sem wait 1
sem signal 1
sem list
```
*Narration : "Les sémaphores permettent de contrôler l'accès à un
nombre limité de ressources."*

---

### PARTIE 7 : Gestion mémoire (1 min)

**[07:30-08:00]**
```
mem stats
mem test
mem map
```
*Narration : "Le gestionnaire mémoire utilise un pool allocator
avec bitmap pour des allocations rapides et déterministes."*

---

### PARTIE 8 : Conclusion (30 sec)

**[08:00-08:30]**
```
info
ps
```
*Narration : "myos-i686 démontre les concepts fondamentaux d'un
système d'exploitation : gestion de tâches, ordonnancement,
communication inter-processus et synchronisation."*

---

## COMMANDES CLÉS POUR LA DÉMO

| Commande | Description |
|----------|-------------|
| `help` | Affiche l'aide complète |
| `info` | Informations système |
| `ps` | Liste des processus |
| `spawn nom` | Crée un processus |
| `block/unblock pid` | Bloque/débloque |
| `sched type` | Change l'ordonnanceur |
| `queue` | Affiche la file READY |
| `log` | Journal d'exécution |
| `demo` | Démo complète |
| `bench n` | Benchmark n processus |
| `mbox` | Gestion mailboxes |
| `mutex` | Gestion mutex |
| `sem` | Gestion sémaphores |
| `mem` | Gestion mémoire |

---

## CONSEILS DE PRÉSENTATION

1. **Parlez lentement** : Laissez le temps de lire l'écran
2. **Faites des pauses** : Entre chaque section
3. **Pointez l'écran** : Si possible, utilisez un surligneur
4. **Préparez les commandes** : Tapez-les en avance si possible
5. **Testez avant** : Faites un essai complet avant l'enregistrement

## SCRIPT DE LANCEMENT RAPIDE

```bash
#!/bin/bash
# demo.sh - Lance la démo

echo "=== Démo myos-i686 ==="
echo "Appuyez sur Entrée pour démarrer..."
read

make clean && make
qemu-system-i386 -kernel kernel/build/myos.bin -m 32M
```
