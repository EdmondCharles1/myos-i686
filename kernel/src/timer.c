/*
 * timer.c - Implementation du timer système (PIT) avec scheduler
 */

#include "timer.h"
#include "irq.h"
#include "printf.h"
#include "types.h"

// =============================================================================
// Ports I/O du PIT
// =============================================================================

#define PIT_CHANNEL0    0x40    // Channel 0 data port
#define PIT_CHANNEL1    0x41    // Channel 1 data port (pas utilisé)
#define PIT_CHANNEL2    0x42    // Channel 2 data port (speaker)
#define PIT_COMMAND     0x43    // Mode/Command register

// =============================================================================
// Variables globales
// =============================================================================

static volatile uint64_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

// Forward declaration pour le scheduler
extern void scheduler_schedule(void);
static bool scheduler_enabled = false;

// =============================================================================
// Fonctions I/O
// =============================================================================

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// =============================================================================
// Handler du timer (appelé à chaque tick)
// =============================================================================

static void timer_handler(registers_t* regs) {
    (void)regs;  // Inutilisé
    timer_ticks++;
    
    // Appeler le scheduler si activé
    if (scheduler_enabled) {
        scheduler_schedule();
    }
}

// =============================================================================
// Implémentation
// =============================================================================

void timer_init(uint32_t frequency) {
    printf("[TIMER] Initialisation du timer a %u Hz...\n", frequency);
    
    // Sauvegarder la fréquence
    timer_frequency = frequency;
    
    // Calculer le diviseur pour obtenir la fréquence souhaitée
    uint32_t divisor = TIMER_FREQUENCY / frequency;
    
    // Envoyer la commande au PIT
    // Channel 0, mode 3 (square wave), binary mode
    outb(PIT_COMMAND, 0x36);
    
    // Envoyer le diviseur (low byte puis high byte)
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    
    // Enregistrer le handler pour IRQ 0 (timer)
    irq_register_handler(0, timer_handler);
    
    printf("[TIMER] Timer initialise (diviseur: %u)\n", divisor);
    printf("[TIMER] Periode: %u ms\n", 1000 / frequency);
}

void timer_enable_scheduler(void) {
    scheduler_enabled = true;
    printf("[TIMER] Scheduler active dans le timer\n");
}

void timer_disable_scheduler(void) {
    scheduler_enabled = false;
    printf("[TIMER] Scheduler desactive dans le timer\n");
}

uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

void timer_wait(uint32_t ticks) {
    uint64_t end_tick = timer_ticks + ticks;
    while (timer_ticks < end_tick) {
        __asm__ volatile ("hlt");
    }
}

uint64_t timer_get_ms(void) {
    if (timer_frequency == 0) {
        return 0;
    }
    return (timer_ticks * 1000) / timer_frequency;
}