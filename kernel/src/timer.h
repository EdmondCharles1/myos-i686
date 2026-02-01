/*
 * timer.h - Programmable Interval Timer (PIT 8253/8254)
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "isr.h"

// =============================================================================
// Constantes
// =============================================================================

// Fréquence de base du cristal du PIT (Oscillateur hardware)
#define TIMER_BASE_FREQUENCY 1193182 

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le timer système (PIT)
 * * @param frequency Fréquence d'interruption souhaitée en Hz
 * (ex: 100 Hz = interruption toutes les 10ms)
 */
void timer_init(uint32_t frequency);

/**
 * Active l'appel du scheduler à chaque tick (Mode Temps Réel)
 */
void timer_enable_scheduler(void);

/**
 * Désactive l'appel du scheduler (Mode Manuel / Simulation)
 * Le temps continue d'avancer (ticks++), mais pas de changement de tâche.
 */
void timer_disable_scheduler(void);

/**
 * Retourne le nombre de ticks depuis le démarrage
 */
uint64_t timer_get_ticks(void);

/**
 * Attend un certain nombre de ticks (Fonction bloquante)
 * * @param ticks Nombre de ticks à attendre
 */
void timer_wait(uint32_t ticks);

/**
 * Retourne le temps écoulé en millisecondes
 */
uint64_t timer_get_ms(void);

#endif // TIMER_H