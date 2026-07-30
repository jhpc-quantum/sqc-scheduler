#pragma once





typedef sqc_result_t (*sqc_session_read_proc_t)(
	const sqc_session_t *sptr,
	void *buf, size_t len,
	ssize_t *actual_read);

typedef sqc_result_t (*sqc_session_write_proc_t)(
	const sqc_session_t *sptr,
	const void *buf, size_t len,
	ssize_t *actual_write);
