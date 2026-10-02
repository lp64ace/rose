#ifndef KER_REPORT_H
#define KER_REPORT_H

#include "DNA_windowmanager_types.h"

struct Report;
struct ReportList;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a #ReportList struct.
 *
 * \note Not thread-safe, should only be called from the 'owner' thread of the report list.
 */
void KER_reports_init(struct ReportList *reports, int flag);
void KER_reports_free(struct ReportList *reports);
void KER_reports_clear(struct ReportList *reports);

void KER_reports_lock(struct ReportList *reports);
void KER_reports_unlock(struct ReportList *reports);

void KER_report(struct ReportList *reports, int type, const char *message);
void KER_reportf(struct ReportList *reports, int type, const char *format, ...);

struct Report *KER_reports_last_displayable(struct ReportList *reports);

#ifdef __cplusplus
}
#endif

#endif // !KER_REPORT_H
