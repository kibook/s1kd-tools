/**
 * @file brexcheck.h
 * @brief Check CSDB objects against BREX data modules
 */

#ifndef S1KD_BREXCHECK
#define S1KD_BREXCHECK

/**
 * BREX check options.
 */
typedef enum {
	S1KD_BREXCHECK_VALUES       = 1,
	S1KD_BREXCHECK_SNS          = 2,
	S1KD_BREXCHECK_SNS_STRICT   = 4,
	S1KD_BREXCHECK_SNS_UNSTRICT = 8,
	S1KD_BREXCHECK_NOTATION     = 16
} s1kdBREXCheckOption;

/**
 * Check a CSDB object against the appropriate S1000D default BREX and generate
 * a report of the results.
 *
 * @param doc The CSDB object
 * @param options A combination of s1kdBREXCheckOption
 * @param report XML report returned by the BREX check. The caller must free the report. If report is NULL, the report is discarded.
 * @return 0 if there are no BREX errors, non-zero otherwise
 */
int s1kdDocCheckDefaultBREX(xmlDocPtr doc, int options, xmlDocPtr *report);

/**
 * Check a CSDB object against the appropriate S1000D default BREX and generate
 * a report of the results.
 *
 * @param object_xml Input buffer containing the XML of the CSDB object
 * @param object_size Size of the object XML buffer
 * @param options A combination of s1kdBREXCheckOption
 * @param report_xml Output buffer for the XML of the BREX report. The caller must free the buffer. If report_xml is NULL, the report is discarded.
 * @param report_size Size of the report XML buffer
 * @return 0 if there are no BREX errors, non-zero otherwise
 */
int s1kdCheckDefaultBREX(const char *object_xml, int object_size, int options, char **report, int *report_size);

/**
 * Check a CSDB object against a BREX data module and generate a report of the
 * results.
 *
 * @param doc The CSDB object
 * @param brex The BREX data module
 * @param options A combination of s1kdBREXCheckOption
 * @param report XML report returned by the BREX check. The caller must free the report. If report is NULL, the report is discarded.
 * @return 0 if there are no BREX error,s non-zero otherwise
 */
int s1kdDocCheckBREX(xmlDocPtr doc, xmlDocPtr brex, int options, xmlDocPtr *report);

/** Check a CSDB object against a BREX data module and generate a report of the
 * results.
 *
 * @param object_xml Input buffer containing the XML of the CSDB object
 * @param object_size Size of the object XML buffer
 * @param brex_xml Input buffer containing the XML of the BREX data module.
 * @param brex_size Size of the BREX buffer
 * @param options A combination of s1kdBREXCheckOption
 * @param report_xml Output buffer for the XML of the BREX report. The caller must free the buffer. If report_xml is NULL, the report is discarded.
 * @param report_size Size of the report XML buffer
 * @return 0 if there are no BREX errors, non-zero otherwise.
 */
int s1kdCheckBREX(const char *object_xml, int object_size, const char *brex_xml, int brex_size, int options, char **report_xml, int *report_size);

#endif
