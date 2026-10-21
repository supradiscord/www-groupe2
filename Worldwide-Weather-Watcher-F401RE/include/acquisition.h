#ifndef ACQUISITION_H
#define ACQUISITION_H

/*
 * Acquisition des capteurs selon le mode :
 *  - STANDARD    : une mesure toutes les LOG_INTERVAL minutes
 *  - ECONOMIQUE  : intervalle x2, GPS une mesure sur deux
 *  - MAINTENANCE : affichage en direct toutes les 5 s (pas d'ecriture SD)
 *  - CONFIG      : acquisition desactivee
 * Fonctionnement NON BLOQUANT : Acq_Update() est appelee en boucle.
 */
void Acq_Init(void);
void Acq_Update(void);

#endif
