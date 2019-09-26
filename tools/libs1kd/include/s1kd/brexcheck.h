/**
 * @file brexcheck.h
 * @brief Check CSDB objects against BREX data modules
 */

#ifndef S1KD_BREXCHECK
#define S1KD_BREXCHECK

/**
 * Check a CSDB object against the appropriate S1000D default BREX
 *
 * @param doc The CSDB object
 * @return 0 if there are no BREX errors, non-zero otherwise
 */
int s1kdCheckDefaultBREX(xmlDocPtr doc);

/**
 * Check a CSDB object against a BREX data module.
 *
 * @param doc The CSDB object
 * @param brex The BREX data module
 * @return 0 if there are no BREX error,s non-zero otherwise
 */
int s1kdCheckBREX(xmlDocPtr doc, xmlDocPtr brex);

#endif
