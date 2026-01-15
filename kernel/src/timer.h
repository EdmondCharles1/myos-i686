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

#define TIMER_FREQUENCY 1193182  // Fréquence de base du PIT (Hz)

// =============================================================================
// API publique
// =============================================================================

/**
 * Initialise le timer système (PIT)
 * 
 * @param frequency Fréquence d'interruption souhaitée en Hz
 *                  (ex: 100 Hz = interruption toutes les 10ms)
 */
void timer_init(uint32_t frequency);

/**
 * Active l'appel du scheduler à chaque tick
 */
void timer_enable_scheduler(void);

/**
 * Désactive l'appel du scheduler
 */
void timer_disable_scheduler(void);

/**
 * Retourne le nombre de ticks depuis le démarrage
 */
uint64_t timer_get_ticks(void);

/**
 * Attend un certain nombre de ticks
 * 
 * @param ticks Nombre de ticks à attendre
 */
void timer_wait(uint32_t ticks);

/**
 * Retourne le temps écoulé en millisecondes
 */
uint64_t timer_get_ms(void);

#endif // TIMER_H