#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "php_tarantool.h"
#include "tarantool_network.h"

void double_to_tv(double tm, struct timeval *tv) {
	tv->tv_sec = floor(tm);
	tv->tv_usec = floor((tm - floor(tm)) * pow(10, 6));
}

void double_to_ts(double tm, struct timespec *ts) {
	ts->tv_sec = floor(tm);
	ts->tv_nsec = floor((tm - floor(tm)) * pow(10, 9));
}

/* `pid` means persistent_id */
// void tntll_stream_close(php_stream *stream, const char *pid) {
void tntll_stream_close(php_stream *stream, zend_string *pid) {
	int rv = PHP_STREAM_PERSISTENT_SUCCESS;
	if (stream == NULL) {
		rv = tntll_stream_fpid2(pid, &stream);
	}
	if (rv == PHP_STREAM_PERSISTENT_SUCCESS && stream != NULL) {
		if (pid) {
			php_stream_free(stream, PHP_STREAM_FREE_CLOSE_PERSISTENT);
		} else {
			php_stream_close(stream);
		}
	}
}

int tntll_stream_fpid2(zend_string *pid, php_stream **ostream) {
	TSRMLS_FETCH();
	return php_stream_from_persistent_id(pid->val, ostream TSRMLS_CC);
}

int tntll_stream_fpid(const char *host, int port, zend_string *pid,
		      php_stream **ostream, char **err) {
	TSRMLS_FETCH();
	*ostream = NULL;
	int rv = 0;
	if (pid) {
		rv = php_stream_from_persistent_id(pid->val, ostream TSRMLS_CC);
		if (rv == PHP_STREAM_PERSISTENT_FAILURE) {
			spprintf(err, 0, "Failed to load persistent stream.");
			return -1;
		}
	}
	if (rv == PHP_STREAM_PERSISTENT_NOT_EXIST) {
		return tntll_stream_open(host, port, pid, ostream, err);
	}
	return 0;
}

int tntll_stream_open(const char *host, int port, zend_string *pid,
		      php_stream **ostream, char **err) {
	TSRMLS_FETCH();
	php_stream *stream = NULL;
	struct timeval tv = {0};
	int errcode = 0, options = 0, flags = 0;
	char  *addr = NULL;
	zend_string *errstr = NULL;
	size_t addr_len = 0;

	addr_len = spprintf(&addr, 0, "tcp://%s:%d", host, port);
	options = REPORT_ERRORS;
	if (pid) options |= STREAM_OPEN_PERSISTENT;
	flags = STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT;
	double_to_tv(TARANTOOL_G(timeout), &tv);
	// printf("timeout: 'sec(%d), usec(%d)'\n",
	//        (int )tv.tv_sec, (int )tv.tv_usec);

	const char *pid_str = pid == NULL ? NULL : pid->val;
	stream = php_stream_xport_create(addr, addr_len, options, flags,
					 pid_str, &tv, NULL, &errstr,
					 &errcode);
	efree(addr);

	if (errcode || !stream) {
		spprintf(err, 0, "Failed to connect [%d]: %s", errcode,
			 ZSTR_VAL(errstr));
		goto error;
	}

	/* Set READ_TIMEOUT */
	double_to_tv(TARANTOOL_G(request_timeout), &tv);
	// printf("request_timeout:  'sec(%d), usec(%d)'\n",
	//        (int )tv.tv_sec, (int )tv.tv_usec);

	if (tv.tv_sec != 0 || tv.tv_usec != 0) {
		php_stream_set_option(stream, PHP_STREAM_OPTION_READ_TIMEOUT,
				      0, &tv);
	}

	/* Set TCP_NODELAY */
	int socketd = ((php_netstream_data_t* )stream->abstract)->socket;
	flags = 1;
	if (setsockopt(socketd, IPPROTO_TCP, TCP_NODELAY, (char *) &flags,
		       sizeof(int))) {
		spprintf(err, 0, "Failed setsockopt [%d]: %s", errno,
			 strerror(errno));
		goto error;
	}
	*ostream = stream;
	return 0;
error:
	if (errstr) zend_string_release(errstr);
	if (stream) tntll_stream_close(NULL, pid);
	return -1;
}

#include <sys/time.h>

/*
 * Legacy rtsisyk code, php_stream_read made right
 * See https://bugs.launchpad.net/tarantool/+bug/1182474
 */
size_t tntll_stream_read2(php_stream *stream, char *buf, size_t size) {
	TSRMLS_FETCH();
	size_t total_size = 0;
	ssize_t read_size = 0;

	while (total_size < size) {
		read_size = php_stream_read(stream, buf + total_size,
					    size - total_size);
		assert(read_size + total_size <= size);
		if (read_size <= 0)
			break;
		total_size += read_size;
	}

	return total_size;
}

bool tntll_stream_is_timedout() {
	int err = php_socket_errno();
	if (err == EAGAIN || err == EWOULDBLOCK || err == EINPROGRESS)
		return 1;
	return 0;
}

int tntll_stream_read(php_stream *stream, char *buf, size_t size) {
	TSRMLS_FETCH();
	return php_stream_read(stream, buf, size);
}

int tntll_stream_send(php_stream *stream, char *buf, size_t size) {
	TSRMLS_FETCH();
	if (php_stream_write(stream, buf, size) != size ||
	    php_stream_flush(stream))
		return -1;
	return 0;
}
