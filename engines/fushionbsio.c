/*
 * fushionbsio engine
 *
 * IO engine using the FushionBS interface.
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libaio.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../fio.h"
#include "../lib/pow2.h"
#include "../optgroup.h"
#include "../lib/memalign.h"
#include "cmdprio.h"

struct fushionbsio_data {
	io_context_t aio_ctx;
	struct io_event *aio_events;
	struct iocb **iocbs;
	struct io_u **io_us;

	struct io_u **io_u_index;

	/*
	 * Basic ring buffer. 'head' is incremented in _queue(), and
	 * 'tail' is incremented in _commit(). We keep 'queued' so
	 * that we know if the ring is full or empty, when
	 * 'head' == 'tail'. 'entries' is the ring size, and
	 * 'is_pow2' is just an optimization to use AND instead of
	 * modulus to get the remainder on ring increment.
	 */
	int is_pow2;
	unsigned int entries;
	unsigned int queued;
	unsigned int head;
	unsigned int tail;

	struct cmdprio cmdprio;
};



FIO_STATIC struct ioengine_ops ioengine = {
	.name			= "fushionbsio",
	.version		= FIO_IOOPS_VERSION,
	.flags			= FIO_ASYNCIO_SYNC_TRIM |
					FIO_ASYNCIO_SETS_ISSUE_TIME,
	.init			= fio_fushionbsio_init,
	.post_init		= fio_fushionbsio_post_init,
	.prep			= fio_fushionbsio_prep,
	.queue			= fio_fushionbsio_queue,
	.commit			= fio_fushionbsio_commit,
	.cancel			= fio_fushionbsio_cancel,
	.getevents		= fio_fushionbsio_getevents,
	.event			= fio_fushionbsio_event,
	.cleanup		= fio_fushionbsio_cleanup,
	.open_file		= generic_open_file,
	.close_file		= generic_close_file,
	.get_file_size		= generic_get_file_size,
	.options		= options,
	.option_struct_size	= sizeof(struct libaio_options),
};

static void fio_init fio_fushionbsio_register(void)
{
	register_ioengine(&ioengine);
}

static void fio_exit fio_fushionbsio_unregister(void)
{
	unregister_ioengine(&ioengine);
}