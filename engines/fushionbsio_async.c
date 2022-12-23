/*
 * fushionbsio async engine
 *
 * IO engine using the FushionBS interface.
 *
 */
#include <stdio.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../fio.h"
#include "../lib/pow2.h"
#include "../optgroup.h"
#include "../lib/memalign.h"
#include "cmdprio.h"
#include "../lib/rand.h"
#include "libcfushion.h"

#define LAST_POS(f)	((f)->engine_pos)

struct fio_fbs_async_iou {
	struct io_u *io_u;
	int io_complete;
};

struct fushionbsio_async_options {
	struct thread_data *td;
	unsigned int userspace_reap;
	struct cmdprio_options cmdprio_options;
	unsigned int nowait;
};

struct fushionbsio_async_data {
	struct iovec *iovecs;
//	struct io_u **io_us;
	unsigned int queued;
	unsigned int events;
	unsigned long queued_bytes;

	unsigned long long last_offset;
	struct fio_file *last_file;
	enum fio_ddir last_ddir;

	struct frand_state rand_state;
	//io_context_t aio_ctx;
	struct io_u **aio_events;

	int is_pow2;
	unsigned int entries;
	unsigned int head;
	unsigned int tail;

	struct cmdprio cmdprio;
};

static void fbs_async_cb(void * blob_id, ssize_t ret, void *data)
{
	struct io_u *io_u = data;
	struct fio_fbs_async_iou *iou = io_u->engine_data;

	iou->io_complete = 1;
}

/*static int fio_io_end(struct thread_data *td, struct io_u *io_u, int ret)
{
	if (io_u->file && ret >= 0 && ddir_rw(io_u->ddir))
		LAST_POS(io_u->file) = io_u->offset + ret;

	if (ret != (int) io_u->xfer_buflen) {
		if (ret >= 0) {
			io_u->resid = io_u->xfer_buflen - ret;
			io_u->error = 0;
			return FIO_Q_COMPLETED;
		} else
			io_u->error = errno;
	}

	if (io_u->error) {
		io_u_log_error(td, io_u);
		td_verror(td, io_u->error, "xfer");
	}

	return FIO_Q_COMPLETED;
}*/

/*static int fio_fushionbsio_async_prep(struct thread_data *td, struct io_u *io_u)
{
	struct fio_file *f = io_u->file;

	if (!ddir_rw(io_u->ddir))
		return 0;

	if (LAST_POS(f) != -1ULL && LAST_POS(f) == io_u->offset)
		return 0;

	if (lseek(f->fd, io_u->offset, SEEK_SET) == -1) {
		td_verror(td, errno, "lseek");
		return 1;
	}

	return 0;
}*/

int fio_fushionbsio_async_open_file(struct thread_data *td,
					  struct io_u *io_u)
{
	struct fio_file *f = io_u->file;
	struct BlobID *ret;

	ret = OpenBlobFS("1", f->fileno, 2);
	if (ret == NULL)
	{
		return -1;
	}
	f->engine_data = ret;
	return 0;

}

int fio_fushionbsio_async_close_file(struct thread_data *td,
					  struct io_u *io_u)
{
	struct fio_file *f = io_u->file;
	struct BlobID *blob_id = f->engine_data;
	int ret;

	ret = CloseBlob(blob_id);
	
	return 0;
}

static enum fio_q_status fio_fushionbsio_async_queue(struct thread_data fio_unused * td,
					    struct io_u *io_u)
{
	struct fio_file *f = io_u->file;
	struct BlobID *blob_id = f->engine_data;
	int ret;

	fio_ro_check(td, io_u);

	if (io_u->ddir == DDIR_READ)
		ret = ReadBlob(blob_id, io_u->xfer_buf, io_u->offset, io_u->xfer_buflen, fbs_async_cb);
	else if (io_u->ddir == DDIR_WRITE)
		ret = WriteBlob(blob_id, io_u->xfer_buf, io_u->xfer_buflen, 0, fbs_async_cb);
	else if (io_u->ddir == DDIR_TRIM) 
		DeleteBlob(blob_id);
	else if (io_u->ddir == DDIR_SYNC)
		ret = SyncBlob(blob_id);
	else
		ret = EINVAL;

	if (ret < 0 || ret == EINVAL) {
	log_err("fushionbsio async queue failed.\n");
	io_u->error = ret;
	goto failed;
	}

	return FIO_Q_QUEUED;

failed:
	io_u->error = ret;
	td_verror(td, io_u->error, "xfer");
	return FIO_Q_COMPLETED;
}

static int fio_fushionbsio_async_init(struct thread_data *td)
{
	struct fushionbsio_async_data *ld;
	struct fushionbsio_async_options *o = td->eo;
	int ret;

	ld = calloc(1, sizeof(*ld));

	ld->entries = td->o.iodepth;
	ld->is_pow2 = is_power_of_2(ld->entries);
	ld->aio_events = calloc(ld->entries, sizeof(struct io_u *));
//	ld->io_us = calloc(ld->entries, sizeof(struct io_u *));

	td->io_ops_data = ld;

	//ret = fio_cmdprio_init(td, &ld->cmdprio, &o->cmdprio_options);
	ret = Init("/etc/fio/config");
	if (ret) {
		td_verror(td, EINVAL, "fio_fushionbsio_async_init");
		return 1;
	}

	return 0;
}


static struct io_u *fio_fushionbsio_async_event(struct thread_data *td, int event)
{
	struct fushionbsio_async_data *fad = td->io_ops_data;

	return fad->aio_events[event];
}

static int fio_fushionbsio_async_getevents(struct thread_data *td, unsigned int min,
			    unsigned int max, const struct timespec *t)
{
	struct fushionbsio_async_data *fad = td->io_ops_data;
	unsigned int events = 0;
	struct io_u *io_u;
	int i;

	dprint(FD_IO, "%s\n", __FUNCTION__);
	do {
		io_u_qiter(&td->io_u_all, io_u, i) {
			struct fio_fbs_async_iou *io;

			if (!(io_u->flags & IO_U_F_FLIGHT))
				continue;

			io = io_u->engine_data;
			if (io->io_complete) {
				io->io_complete = 0;
				fad->aio_events[events] = io_u;
				events++;

				if (events >= max)
					break;
			}

		}
		if (events < min)
			usleep(100);
		else
			break;

	} while (1);

	return events;
}

static void fio_fushionbsio_async_cleanup(struct thread_data *td)
{
	struct fushionbsio_async_data *fad = td->io_ops_data;

	if (fad) {
		free(fad->aio_events);
		free(fad);
	}
}

static struct ioengine_ops ioengine = {
	.name			= "fushionbsio_async",
	.version		= FIO_IOOPS_VERSION,
	.flags			= FIO_ASYNCIO_SYNC_TRIM |
					FIO_ASYNCIO_SETS_ISSUE_TIME,
	//.prep			= fio_fushionbsio_async_prep,
	.queue			= fio_fushionbsio_async_queue,
	.open_file		= fio_fushionbsio_async_open_file,
	.close_file		= fio_fushionbsio_async_close_file,
	.init			= fio_fushionbsio_async_init,
	.getevents 		= fio_fushionbsio_async_getevents,
	.event 			= fio_fushionbsio_async_event,
	.cleanup		= fio_fushionbsio_async_cleanup,
};

static void fio_init fio_fushionbsio_register(void)
{
	register_ioengine(&ioengine);
}

static void fio_exit fio_fushionbsio_unregister(void)
{
	unregister_ioengine(&ioengine);
}