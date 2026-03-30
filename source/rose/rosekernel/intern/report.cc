#include "MEM_guardedalloc.h"

#include "KER_report.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_thread.h"
#include "LIB_utildefines.h"

#include <stdio.h>

void KER_reports_init(ReportList *reports, int flag) {
	if (!reports) {
		return;
	}

	memset(reports, 0, sizeof(ReportList));

	reports->storelevel = RPT_INFO;
	reports->printlevel = RPT_ERROR;
	reports->flag = flag;

	reports->lock = static_cast<void *>(LIB_mutex_alloc());
}

void KER_reports_free(ReportList *reports) {
	if (!reports) {
		return;
	}

	KER_reports_clear(reports);

	LIB_mutex_free(static_cast<ThreadMutex *>(reports->lock));
	reports->lock = NULL;
}

void KER_reports_clear(ReportList *reports) {
	if (!reports) {
		return;
	}

	KER_reports_lock(reports);

	Report *report = static_cast<Report *>(reports->reports.first);
	while (report) {
		Report *next = report->next;
		MEM_freeN(const_cast<char *>(report->message));
		MEM_freeN(report);
		report = next;
	}

	LIB_listbase_clear(&reports->reports);

	KER_reports_unlock(reports);
}

void KER_reports_lock(ReportList *reports) {
	LIB_mutex_lock(static_cast<ThreadMutex *>(reports->lock));
}

void KER_reports_unlock(ReportList *reports) {
	LIB_mutex_unlock(static_cast<ThreadMutex *>(reports->lock));
}

const char *KER_report_type_str(int type) {
	/* clang-format off */
	
	switch (type) {
		case RPT_DEBUG: return "Debug";
		case RPT_INFO: return "Info";
		case RPT_WARNING: return "Warning";
		case RPT_ERROR: return "Error";
	}

	/* clang-format on */

	return "Undefined Type";
}

void KER_report(ReportList *reports, int type, const char *message) {
	if (reports && (type >= reports->printlevel)) {
		fprintf(stdout, "%s\n");
	}

	if (reports && (reports->flag & RPT_STORE) && (type >= reports->storelevel)) {
		KER_reports_lock(reports);

		Report *report = static_cast<Report *>(MEM_callocN(sizeof(Report), "Report"));
		report->type = type;
		report->length = LIB_strlen(message);
		report->message = LIB_strndupN(message, report->length);
		report->typestr = KER_report_type_str(type);

		LIB_addtail(&reports->reports, report);

		KER_reports_unlock(reports);
	}
}

void KER_reportf(ReportList *reports, int type, const char *fmt, ...) {
	va_list args;

	if (reports && (type >= reports->printlevel)) {
		va_start(args, fmt);
		vfprintf(stdout, fmt, args);
		fprintf(stdout, "\n");
		va_end(args);
	}

	if (reports && (reports->flag & RPT_STORE) && (type >= reports->storelevel)) {
		KER_reports_lock(reports);

		Report *report = static_cast<Report *>(MEM_callocN(sizeof(Report), "Report"));
		report->type = type;
		report->typestr = KER_report_type_str(type);

		va_start(args, fmt);
		report->message = LIB_vstrformat_allocN(fmt, args);
		report->length = LIB_strlen(report->message);
		va_end(args);

		LIB_addtail(&reports->reports, report);

		KER_reports_unlock(reports);
	}
}

Report *KER_reports_last_displayable(ReportList *reports) {
	Report *report = NULL;

	KER_reports_lock(reports);
	
	LISTBASE_FOREACH_BACKWARD(Report *, itr, &reports->reports) {
		if (ELEM(itr->type, RPT_ERROR, RPT_WARNING, RPT_INFO)) {
			report = itr;
			break;
		}
	}

	KER_reports_unlock(reports);

	return report;
}
