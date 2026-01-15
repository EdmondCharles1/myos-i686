/*
 * stack_protector.c - Implementation du Stack Smashing Protector
 */

#include "stack_protector.h"
#include "printf.h"

// =============================================================================
// Valeur du canary
// =============================================================================

/**
 * Valeur du garde (canary)
 * 
 * GCC insère automatiquement cette valeur sur la pile et la vérifie
 * avant chaque retour de fonction.
 * 
 * IMPORTANT : Cette variable doit être accessible globalement car GCC
 * y accède directement dans le code généré.
 */
#if UINT32_MAX == UINTPTR_MAX
    // Architecture 32-bit
    uintptr_t __stack_chk_guard = 0xDEADBEEF;
#else
    // Architecture 64-bit (futur support)
    uintptr_t __stack_chk_guard = 0xDEADBEEFCAFEBABE;
#endif

// =============================================================================
// Initialisation
// =============================================================================

void stack_protector_init(void) {
    // Pour un kernel plus robuste, on pourrait générer une valeur aléatoire ici
    // En utilisant par exemple :
    // - RDRAND (instruction CPU si disponible)
    // - Timer + PIT pour obtenir de l'entropie
    // - Combinaison de plusieurs sources
    
    // Pour l'instant, on garde la valeur statique
    printf("[SSP] Stack Smashing Protector active\n");
    printf("[SSP] Canary: 0x%X\n", __stack_chk_guard);
}

// =============================================================================
// Handler de détection
// =============================================================================

/**
 * Fonction appelée par GCC quand un stack smashing est détecté
 * 
 * Cette fonction ne retourne JAMAIS.
 */
void __stack_chk_fail(void) {
    // Désactiver les interruptions pour éviter toute corruption supplémentaire
    __asm__ volatile ("cli");
    
    // Afficher un message d'erreur critique
    printf("\n");
    printf("=====================================\n");
    printf("   STACK SMASHING DETECTED !!!\n");
    printf("=====================================\n");
    printf("\n");
    printf("Un buffer overflow a ete detecte.\n");
    printf("Le kernel va s'arreter pour eviter\n");
    printf("toute corruption supplementaire.\n");
    printf("\n");
    printf("Valeur du canary attendue: 0x%X\n", __stack_chk_guard);
    printf("\n");
    printf("Kernel HALTED.\n");
    
    // Arrêter le kernel définitivement
    while (1) {
        __asm__ volatile ("hlt");
    }
}

/**
 * Alias pour __stack_chk_fail
 * 
 * Certaines versions de GCC appellent cette variante.
 */
void __stack_chk_fail_local(void) {
    __stack_chk_fail();
}
