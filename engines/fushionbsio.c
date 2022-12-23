/*
 * fushionbsio engine
 *
 * IO engine using the FushionBS interface.
 *
 */
#include <stdio.h>
#include <sys/uio.h>
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
#include "../lib/rand.h"
#include "libcfushion.h"

#define LAST_POS(f)	((f)->engine_pos)

struct fushionbsio_data {
	struct iovec *iovecs;
	struct io_u **io_us;
	unsigned int queued;
	unsigned int events;
	unsigned long queued_bytes;

	unsigned long long last_offset;
	struct fio_file *last_file;
	enum fio_ddir last_ddir;

	struct frand_state rand_state;
};

static int fio_io_end(struct thread_data *td, struct io_u *io_u, int ret)
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
}

static enum fio_q_status fio_fushionbsio_queue(struct thread_data *td,
					  struct io_u *io_u)
{
	struct fio_file *f = io_u->file;
	struct BlobID *blob_id = f->engine_data;
	int ret;

	fio_ro_check(td, io_u);

	if (io_u->ddir == DDIR_READ)
		/*ret = read(f->fd, io_u->xfer_buf, io_u->xfer_buflen);*/
		ret = ReadBlob(blob_id, io_u->xfer_buf, io_u->offset, io_u->xfer_buflen, NULL);
	else if (io_u->ddir == DDIR_WRITE)
		/*ret = write(f->fd, io_u->xfer_buf, io_u->xfer_buflen);*/
		ret = WriteBlob(blob_id, io_u->xfer_buf, io_u->xfer_buflen, 0, NULL);
	else if (io_u->ddir == DDIR_TRIM) {
		/*do_io_u_trim(td, io_u);*/
		DeleteBlob(blob_id);
		return FIO_Q_COMPLETED;
	} else
		/*ret = do_io_u_sync(td, io_u);*/
		ret = SyncBlob(blob_id);

	return fio_io_end(td, io_u, ret);
}

static int fio_fushionbsio_prep(struct thread_data *td, struct io_u *io_u)
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
}

int fio_fushionbsio_open_file(struct thread_data *td,
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

int fio_fushionbsio_close_file(struct thread_data *td,
					  struct io_u *io_u)
{
	struct fio_file *f = io_u->file;
	struct BlobID *blob_id = f->engine_data;
	int ret;

	ret = CloseBlob(blob_id);
	
	return 0;
}

static int fio_fushionbsio_init(struct thread_data *td)
{
	struct fushionbsio_data *ld;
	//struct fushionbsio_options *o = td->eo;
	int ret;

	ld = calloc(1, sizeof(*ld));

	/*ld->entries = td->o.iodepth;
	ld->is_pow2 = is_power_of_2(ld->entries);
	ld->aio_events = calloc(ld->entries, sizeof(struct io_u *));*/


	td->io_ops_data = ld;

	ret = Init("/etc/fio/config");
	if (ret) {
		td_verror(td, EINVAL, "fio_fushionbsio_init");
		return 1;
	}

	return 0;
}

static struct ioengine_ops ioengine = {
	.name			= "fushionbsio",
	.version		= FIO_IOOPS_VERSION,
	.flags			= FIO_SYNCIO,
	.prep			= fio_fushionbsio_prep,
	.queue			= fio_fushionbsio_queue,
	.open_file		= fio_fushionbsio_open_file,
	.close_file		= fio_fushionbsio_close_file,
	.init			= fio_fushionbsio_init,
};

static void fio_init fio_fushionbsio_register(void)
{
	register_ioengine(&ioengine);
}

static void fio_exit fio_fushionbsio_unregister(void)
{
	unregister_ioengine(&ioengine);
}