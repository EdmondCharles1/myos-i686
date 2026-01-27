# Scenarios de Test - myos-i686

Ce document decrit les scenarios de test pour valider toutes les fonctionnalites du systeme.

## Pre-requis

1. Compiler le projet: `make`
2. Lancer QEMU: `make run`
3. Attendre le shell interactif

---

## 1. Tests de Base

### 1.1 Verification du demarrage
```
# Le systeme doit demarrer et afficher le shell
# Attendu: prompt "myos-i686 shell >"
```

### 1.2 Commandes basiques
```
help
# Attendu: Liste de toutes les commandes disponibles

info
# Attendu: Informations systeme (version, ordonnanceur, etc.)

uptime
# Attendu: Temps d'execution en heures/minutes/secondes
```

---

## 2. Tests de Gestion des Processus

### 2.1 Liste des processus
```
ps
# Attendu: Au moins le processus "idle" avec PID, etat, priorite
```

### 2.2 Creation de processus
```
spawn 3 50
# Attendu: "Creation de 3 processus (burst=50)..."
# Puis: "3 processus crees"

ps
# Attendu: 4 processus (idle + 3 nouveaux)
```

### 2.3 Terminaison de processus
```
ps
# Noter un PID (ex: 2)

kill 2
# Attendu: "Processus termine avec succes"

ps
# Attendu: Le processus n'apparait plus
```

### 2.4 Transitions d'etats
```
states
# Attendu: Creation d'un processus "StateDemo"
# Affiche: etat NEW puis READY

ps
# Attendu: Processus StateDemo en READY

block <PID>
# Attendu: "Processus PID=X bloque"

ps
# Attendu: Processus en etat BLOCKED

unblock <PID>
# Attendu: "Processus PID=X debloque"

ps
# Attendu: Processus en etat READY
```

---

## 3. Tests d'Ordonnancement

### 3.1 Changement d'ordonnanceur
```
sched
# Attendu: Type actuel (Round Robin par defaut)

sched fcfs
# Attendu: "Ordonnanceur: FCFS (non preemptif)"

sched rr
# Attendu: "Ordonnanceur: Round Robin (quantum=10 ticks)"

sched priority
# Attendu: "Ordonnanceur: Priority (preemptif)"

sched sjf
# Attendu: "Ordonnanceur: SJF (non preemptif)"

sched srtf
# Attendu: "Ordonnanceur: SRTF (preemptif)"

sched mlfq
# Attendu: "Ordonnanceur: MLFQ (3 niveaux)"
```

### 3.2 Test Round Robin
```
sched rr
spawn 3 100
queue
# Attendu: File READY avec 3 processus

# Attendre quelques secondes...
log
# Attendu: Entrées de journal montrant context switches
```

### 3.3 Test MLFQ
```
sched mlfq
spawn 3 100
queue
# Attendu: 3 niveaux (0, 1, 2) avec quantum different

# Attendre la demotion...
queue
# Attendu: Processus migrent vers niveaux inferieurs
```

### 3.4 Test SJF/SRTF
```
sched sjf
bench
# Attendu: 3 processus avec burst differents (20, 50, 100)

queue
# Attendu: Processus tries par burst_time (Short en premier)
```

### 3.5 Test Priority
```
sched priority
demo
# Attendu: 4 processus avec priorites variees (30, 20, 10, 5)

queue
# Attendu: Processus tries par priorite (HighPrio en premier)
```

---

## 4. Tests de Memoire

### 4.1 Statistiques memoire
```
mem stats
# Attendu: Pool 64KB, 1024 blocs, utilisation
```

### 4.2 Test d'allocation
```
mem test
# Attendu:
# - Test 1: Allocations simples (3 pointeurs)
# - Test 2: Liberation
# - Test 3: Reallocation
# - Test 4: kcalloc
# - Stats finales
```

### 4.3 Bitmap memoire
```
mem bitmap
# Attendu: Affichage des premiers 64 blocs (. = libre, # = utilise)
```

---

## 5. Tests IPC (Mailboxes)

### 5.1 Creation de mailbox
```
mbox create test
# Attendu: "Mailbox 'test' creee (ID=1)"

mbox list
# Attendu: Tableau avec mailbox "test"
```

### 5.2 Envoi/Reception de messages
```
mbox send 1 Hello
# Attendu: "Message envoye"

mbox recv 1
# Attendu: "Message recu: 'Hello' (de PID=0)"

mbox recv 1
# Attendu: "Mailbox vide"
```

### 5.3 Test complet IPC
```
mbox test
# Attendu:
# - Test 1: Creation de mailbox
# - Test 2: Envoi de message
# - Test 3: Comptage
# - Test 4: Reception de message
# - Test 5: Reception sur mailbox vide
# - Test 6: Envoi multiple (remplissage)
# - Test 7: Find par nom
# - Test 8: Destruction
```

---

## 6. Tests de Synchronisation

### 6.1 Test Mutex
```
mutex create mon_mutex
# Attendu: "Mutex 'mon_mutex' cree (ID=1)"

mutex lock 1
# Attendu: "Mutex verrouille"

mutex lock 1
# Attendu: "Mutex deja pris"

mutex unlock 1
# Attendu: "Mutex deverrouille"

mutex list
# Attendu: Tableau avec mutex "mon_mutex" (FREE)
```

### 6.2 Test complet Mutex
```
mutex test
# Attendu:
# - Test 1: Creation
# - Test 2: Lock/Unlock
# - Test 3: Trylock
# - Test 4: Recherche par nom
```

### 6.3 Test Semaphore
```
sem create mon_sem 3
# Attendu: "Semaphore 'mon_sem' cree (ID=1, value=3)"

sem wait 1
# Attendu: "Wait OK, nouvelle valeur: 2"

sem wait 1
# Attendu: "Wait OK, nouvelle valeur: 1"

sem wait 1
# Attendu: "Wait OK, nouvelle valeur: 0"

sem wait 1
# Attendu: "Semaphore a 0 (bloquerait)"

sem post 1
# Attendu: "Post OK, nouvelle valeur: 1"

sem list
# Attendu: Tableau avec semaphore "mon_sem" (value=1)
```

### 6.4 Test complet Semaphore
```
sem test
# Attendu:
# - Test 1: Creation de semaphore
# - Test 2: Wait (decremente)
# - Test 3: Trywait sur semaphore a 0
# - Test 4: Post (incremente)
```

---

## 7. Scenarios de Demo Complete

### 7.1 Demo ordonnancement complet
```
# Tester chaque ordonnanceur avec des processus
sched rr
demo
log
queue

sched mlfq
demo
queue

sched priority
demo
queue

sched sjf
bench
queue
```

### 7.2 Demo IPC et Sync
```
# Creer des ressources
mbox create channel1
mutex create lock1
sem create counter 5

# Utiliser
mbox send 1 TestMessage
mbox recv 1
mutex lock 1
mutex unlock 1
sem wait 1
sem post 1

# Verifier l'etat
mbox list
mutex list
sem list
```

---

## 8. Tests de Robustesse

### 8.1 Commandes invalides
```
commande_inexistante
# Attendu: "Commande inconnue: commande_inexistante"

kill 9999
# Attendu: "Echec: processus introuvable"

mbox recv 9999
# Attendu: "Erreur: -3" (NOTFOUND)
```

### 8.2 Limites du systeme
```
# Creer beaucoup de processus
spawn 10 50
spawn 10 50
spawn 10 50
ps
# Attendu: Max 32 processus, erreur si depasse
```

---

## Resume des Commandes

| Categorie | Commandes |
|-----------|-----------|
| Base | help, clear, info, uptime, reboot |
| Processus | ps, kill, spawn, bench, demo, states, block, unblock |
| Ordonnancement | sched, log, queue |
| Memoire | mem test/stats/bitmap |
| IPC | mbox list/create/send/recv/test |
| Mutex | mutex list/create/lock/unlock/test |
| Semaphore | sem list/create/wait/post/test |

---

## Criteres de Validation

Pour chaque test:
- [ ] La commande s'execute sans crash
- [ ] Le resultat correspond a l'attendu
- [ ] Les etats/valeurs sont coherents
- [ ] Le systeme reste stable apres le test
