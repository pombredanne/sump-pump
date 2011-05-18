/* sump.c - SUMP Pump(TM) SMP/CMP parallel data pump library.
 *          SUMP Pump is a trademark of Ordinal Technology Corp
 *
 * $Revision$
 *
 * Copyright (C) 2010, Ordinal Technology Corp, http://www.ordinal.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Linking SUMP Pump statically or dynamically with other modules is
 * making a combined work based on SUMP Pump.  Thus, the terms and
 * conditions of the GNU General Public License v.2 cover the whole
 * combination.
 *
 * In addition, as a special exception, the copyright holders of SUMP Pump
 * give you permission to combine SUMP Pump program with free software
 * programs or libraries that are released under the GNU LGPL and with
 * independent modules that communicate with SUMP Pump solely through
 * Ordinal Technology Corp's Nsort Subroutine Library interface as defined
 * in the Nsort User Guide, http://www.ordinal.com/NsortUserGuide.pdf.
 * You may copy and distribute such a system following the terms of the
 * GNU GPL for SUMP Pump and the licenses of the other code concerned,
 * provided that you include the source code of that other code when and
 * as the GNU GPL requires distribution of source code.
 *
 * Note that people who make modified versions of SUMP Pump are not
 * obligated to grant this special exception for their modified
 * versions; it is their choice whether to do so.  The GNU General
 * Public License gives permission to release a modified version without
 * this exception; this exception also makes it possible to release a
 * modified version which carries forward this exception.
 * 
 * For more information on SUMP Pump, see:
 *     http://www.ordinal.com/sump.html
 *     http://code.google.com/p/sump-pump/
 */
#define _GNU_SOURCE
#include "sump.h"
#include "sumpversion.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sched.h>
#include <aio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdint.h>

#include "nsort.h"
#include "nsorterrno.h"

#define TRUE 1
#define FALSE 0
#define REC_TYPE(sp) ((sp)->flags & SP_REC_TYPE_MASK)
#define SP_SORT SP_RESERVED_1
#define SP_REDUCE_BY_KEYS SP_RESERVED_2

/* sort_state values */
#define SORT_INPUT      1
#define SORT_OUTPUT     2
#define SORT_DONE       3

#define ERROR_BUF_SIZE  500   /* size of error buffer */

static int Page_size = 4096;

typedef uint64_t        u8;
typedef int64_t         i8;


/* state structure for a sump pump instance */
struct sump
{
    unsigned            flags;         /* caller-defined bit flags,
                                        * see sump.h */
    sp_pump_t           pump_func;     /* caller-defined pump function that 
                                        * is executed in parallel by the
                                        * sump pump */
    void                *pump_arg;     /* caller-defined arg to pump func */
    int                 num_tasks;     /* number of pump tasks */
    int                 num_threads;   /* number of threads executing the
                                        * pump func */
    int                 num_in_bufs;   /* number of input buffers */
    int                 num_outputs;   /* number of sump pump output channels*/
    int                 group_by_keys; /* number of keys to group/reduce
                                        * records by */
    ssize_t             in_buf_size;   /* input buffer size in bytes */
    struct sump_out     *out;          /* array of output structures, one for
                                        * each output */
    void                *delimiter;    /* record delimiter, for text input */
    size_t              rec_size;      /* record size */
    pthread_mutex_t     sump_mtx;      /* mutex for sump pump infrastructure */
    pthread_mutex_t     sp_mtx;        /* mutex for pump funcs to use
                                        * via sp_mutex_lock() and
                                        * sp_mutex_unlock() calls */
    pthread_cond_t      in_buf_readable_cond; /* input buffer available for
                                               * reading by pump funcs */
    pthread_cond_t      in_buf_done_cond;  /* an input buffer has been
                                            * completely read by all potential
                                            * pump threads and can be reused */
    pthread_cond_t      task_avail_cond;   /* a task is available for the
                                            * taking by a sump pump thread */
    pthread_cond_t      task_drained_cond; /* a task has been completely 
                                            * executed and its output has
                                            * been drained (read) for its
                                            * output buffer(s) */
    pthread_cond_t      task_output_ready_cond; /* a task's output ready to
                                                 * be read */
    pthread_cond_t      task_output_empty_cond; /* a task's output buffer has
                                                 * been read and is empty */
    size_t              in_buf_current_bytes; /* bytes in current input buf */
    /* number of bytes at end of prev input buffer containing a partial rec */
    size_t              prev_in_buf_ending_rec_partial_bytes;
    pthread_t           *thread;        /* array of sump pump threads */
    u8                  cnt_in_buf_readable; /* number of input buffers that
                                              * have been filled with input
                                              * data and are available for
                                              * reading by sump pump threads
                                              * executing pump functions */
    u8                  cnt_in_buf_done; /* number of input buffers
                                          * that have been read by all
                                          * their readers */
    u8                  cnt_task_init;  /* number of tasks initialized and
                                         * available for the taking by any
                                         * member thread */
    u8                  cnt_task_begun; /* number of tasks allocated/taken
                                         * and begun by member threads */
    u8                  cnt_task_drained; /* number of tasks that have
                                           * been completed and had all
                                           * their output buffer(s)
                                           * completely read/drained. */
    u8                  cnt_task_done; /* number of done tasks whose
                                        * actual ending position has been
                                        * verified to be the same as their
                                        * expected ending */
    struct sp_task      *task;          /* array of sump pump tasks */
    struct in_buf       *in_buf;        /* array of sump pump input buffers */
    nsort_t             nsort_ctx;      /* used only if this is a sort */
    char                *error_buf;     /* buf to hold error msg */
    size_t              error_buf_size; /* buf to hold error msg */
    int                 error_code;     /* pump func generated error code */
    unsigned            sort_error;     /* sort error code */
    char                *sort_temp_buf; /* sort temporary buf */
    size_t              sort_temp_buf_size; /* size of sort temporary buf */
    size_t              sort_temp_buf_bytes; /* bytes of data in temp buf */
    char                input_eof;      /* sp_write_input() called with
                                         * size <= 0 */
    char                broken_input;   /* sp_write_input() called with
                                         * a negative size */
    char                sort_state;     /* only used for sorting */
    const char          *in_file;       /* input file str or NULL if none */
    struct sp_file      *in_file_sp;    /* input file of sump pump */
};

/* struct for an output of a task */
struct task_out
{
    char        *buf;       /* output buffer where map task should
                             * write its results */
    size_t      size;       /* capacity of output buffer */
    size_t      bytes_copied; /* number of bytes written so far into
                               * output buffer */
    char        stalled;  /* the map thread handling this task is
                           * stalled waiting for the writer thread to
                           * empty its full buf */
};

/* struct for a sump pump task */
struct sp_task
{
    struct sump *sp;            /* the "host" sp_t of this task */
    u8          task_number;    /* task number */
    int         thread_index;   /* id of thread performing this task */
    char        *in_buf;        /* input buffer */
    size_t      in_buf_bytes;   /* number of bytes written into in_buf
                                 * by the reader thread.  these bytes
                                 * will be read out by map task */
    char        *rec_buf;       /* buf to hold rec returned by pf_get_rec() */
    size_t      rec_buf_size;   /* size of rec_buf */
    char        *curr_rec;      /* pointer to the current record */
    char        *temp_buf;       /* temp buf to help with printf */
    size_t      temp_buf_size;   /* size of the temp buf */
    char        *error_buf;      /* error message posted by sp_error() call */
    size_t      error_buf_size;  /* size of the error message buf */
    int         error_code;      /* pump func generated error code */
    int         sort_error;      /* nsort error code */
    u8          curr_in_buf_index; /* current input buffer index */
    char        *begin_rec;      /* pointer to the beginning record */
    u8          begin_in_buf_index;/* beginning input buffer index */
    /* the following 2 members are set by the thread calling sp_write_input()
     */
    u8          expected_end_index; /* expected end in buf index */
    int         expected_end_offset; /* expected end in buf offset */
    
    char        first_group_rec; /* next record read will be the first
                                  * record for its record group */
    char        first_in_buf;     /* this task is still reading its
                                   * first input buffer */
    char        input_eof;       /* boolean: this task is done reading
                                  * its input */
    char        output_eof;      /* boolean: thread performing task is
                                  * done writing its output to its output
                                  * buffer, but we may still need to wait
                                  * until the output buffer has been read
                                  * before the task is done */
    int         outs_drained;   /* number of outputs for this task that
                                 * have been completely drained (read) */
    struct task_out *out;       /* array of task outputs */
};

/* struct for a sump pump input buffer */
typedef struct in_buf
{
    char        *in_buf;        /* input buffer */
    size_t      in_buf_bytes;   /* number of bytes written into in_buf
                                 * by the reader thread.  these bytes
                                 * will be read out by map task */
    size_t      in_buf_size;    /* size of the in_buf */
    int         num_readers;    /* number of threads performing
                                 * tasks that read this buf */
    int         num_readers_done; /* number of reader threads that are
                                   * done with this buffer */
} in_buf_t;

/* struct for a link (copy thread) between an output of one sump pump and
 * the input of another.
 */
struct sp_link
{
    struct sump         *out_sp;        /* sp_t we are reading from */
    unsigned            out_index;      /* output index of read sp_t */
    struct sump         *in_sp;         /* sp_t we are writing to */
    size_t              buf_size;       /* buf size */
    char                *buf;           /* temp buf for transfering data */
    pthread_t           thread;         /* thread executing link_main() */
    int                 error_code;     /* error code */
};

/* struct for a file reader or writer thread */
struct sp_file
{
    char        *fname;         /* file name */
    pthread_t   thread;         /* thread executing either file_reader() or
                                 * file_writer() */
    sp_t        sp;             /* sump pump this file */
    int         mode;           /* file access mode */
    int         fd;             /* file descriptor */
    int         out_index;      /* sump pump output index (if relevant) */
    int         aio_count;      /* the max and target number of async i/o's */
    size_t      transfer_size;  /* read or write request size */
    int         error_code;     /* error code */
};

/* file access modes */
#define MODE_UNSPECIFIED    0   /* no access mode has been specified */
#define MODE_BUFFERED       1   /* use standard read() or write() calls */
#define MODE_DIRECT         2   /* direct and asynchronous r/w requests */


/* struct for a sump pump output */
struct sump_out
{
    size_t              buf_size;      /* size of each task's corresponding
                                        * output buffer for this output */
    size_t              partial_bytes_copied; /* the number of bytes copied
                                               * from the task output buffer
                                               * currently being read from */
    const char          *file;         /* output file str or NULL if none */
    struct sp_file      *file_sp;      /* output file for this sump pump out */
    u8                  cnt_task_drained; /* number of tasks that have
                                           * been completed and their
                                           * output buffer for this
                                           * particular output has been
                                           * completely read */
};

/* sump aio struct */ 
struct sump_aio
{
    u8                  buf_index;      /* sump pump buffer index */
    size_t              buf_offset;     /* beginning io offset within buffer */
    char                last_buf_io;    /* boolean indicating last io for buf*/
    off_t               file_offset;    /* file offset */
    size_t              nbytes;         /* request size */
    struct aiocb        aio;
};


/* global sump pump mutex */
static pthread_mutex_t Global_lock = PTHREAD_MUTEX_INITIALIZER;

/* file descriptor for /dev/zero */
static int Zero_fd;

/* default size for sp_write_input() and sp_read_output() transfers for
 * regression testing of those interfaces.
 */
static size_t Default_rw_test_size;

/* default file access mode */
static int Default_file_mode = MODE_BUFFERED;

/* function pointers to nsort library entry points.  These are 
 * non-NULL if the nsort library is linked in.
 */
nsort_msg_t (*Nsort_define)(const char *def,
                            unsigned options,
                            nsort_error_callback_t *callbacks,
                            nsort_t *ctxp);
nsort_msg_t (*Nsort_release_recs)(void *buf, 
                                  size_t size, 
                                  nsort_t *ctxp);
nsort_msg_t (*Nsort_release_end)(nsort_t *ctxp);
nsort_msg_t (*Nsort_return_recs)(void *buf,
                                 size_t *size, 
                                 nsort_t *ctxp);
nsort_msg_t (*Nsort_end)(nsort_t *ctxp);
const char *(*Nsort_get_stats)(nsort_t *ctxp);
char *(*Nsort_message)(nsort_t *ctxp);



static FILE    *TraceFp;
#define TRACE if (TraceFp != NULL) trace

/* trace - print a trace message
 */
static void trace(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    vfprintf(TraceFp, fmt, ap);
    fflush(TraceFp);
}


/* sp_get_version - get the subversion version for sump pump
 */
const char *sp_get_version(void)
{
    return (sp_version);
}


/* sp_get_id - get the subversion id keyword substitution for sump pump
 */
const char *sp_get_id(void)
{
    return ("$Id$");
}


/* die - quit program due to a fatal sump pump infrastructure error.
 */
static void die(char *fmt, ...)
{
    va_list     ap;
    int         ret;

    fprintf(stderr, "sump pump fatal error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}


/* get_nsort_syms - dynamically link to nsort library
 */
static int get_nsort_syms()
{
    void        *syms;
    
    if ((syms = dlopen("libnsort.so", RTLD_GLOBAL | RTLD_LAZY)) == NULL)
        return (-1);
    if ((Nsort_define = dlsym(syms, "nsort_define")) == NULL)
        return (-2);
    if ((Nsort_release_recs = dlsym(syms, "nsort_release_recs")) == NULL)
        return (-2);
    if ((Nsort_release_end = dlsym(syms, "nsort_release_end")) == NULL)
        return (-2);
    if ((Nsort_return_recs = dlsym(syms, "nsort_return_recs")) == NULL)
        return (-2);
    if ((Nsort_end = dlsym(syms, "nsort_end")) == NULL)
        return (-2);
    if ((Nsort_get_stats = dlsym(syms, "nsort_get_stats")) == NULL)
        return (-2);
    if ((Nsort_message = dlsym(syms, "nsort_message")) == NULL)
        return (-2);
    return (0);
}


/* init_zero_fd - initialize Zero_fd, the file descriptor for /dev/zero.
 */
static void init_zero_fd()
{
    pthread_mutex_lock(&Global_lock);
    if (Zero_fd <= 0)
        Zero_fd = open("/dev/zero", O_RDWR);
    pthread_mutex_unlock(&Global_lock);
    return;
}


/* start_error - raise an error and set error message during sp_start()
 *               but before the sump pump threads are created.
 */
static int start_error(sp_t sp, const char *fmt, ...)
{
    va_list     ap;
    int         ret;

    if (sp->error_code != 0)          /* if prior error */
        return sp->error_code;        /* ignore this one */
    sp->error_code = SP_START_ERROR;
    
    va_start(ap, fmt);
    ret = vsnprintf(sp->error_buf, sp->error_buf_size, fmt, ap);
    if (ret >= sp->error_buf_size)
    {
        if (sp->error_buf_size != 0)
            free(sp->error_buf);
        sp->error_buf_size = ret + 1;
        sp->error_buf = (char *)malloc(sp->error_buf_size);
        va_start(ap, fmt);
        vsnprintf(sp->error_buf, sp->error_buf_size, fmt, ap);
    }
    
    return SP_START_ERROR;
}


/* link_in_nsort - if not already done, dynamically link in the nsort library.
 */
int link_in_nsort()
{
    int ret;
    
    pthread_mutex_lock(&Global_lock);
    ret = Nsort_define == NULL ? get_nsort_syms() : 0;
    pthread_mutex_unlock(&Global_lock);
    return (ret);
}    


/* sp_raise_error - raise an error for a sump pump
 */
void sp_raise_error(sp_t sp, int error_code, const char *fmt, ...)
{
    va_list     ap;
    int         ret;

    pthread_mutex_lock(&sp->sump_mtx);
    if (sp->error_code != 0)            /* if prior error */
    {
        pthread_mutex_unlock(&sp->sump_mtx);
        return;                /* ignore this one. let prior error stand */
    }
    sp->error_code = error_code;
    
    va_start(ap, fmt);
    ret = vsnprintf(sp->error_buf, sp->error_buf_size, fmt, ap);
    if (ret >= sp->error_buf_size)
    {
        if (sp->error_buf_size != 0)
            free(sp->error_buf);
        sp->error_buf_size = ret + 1;
        sp->error_buf = (char *)malloc(sp->error_buf_size);
        va_start(ap, fmt);
        vsnprintf(sp->error_buf, sp->error_buf_size, fmt, ap);
    }
    /* Wake up all possible waiting threads for the sump pump.
     * Some of the below pthread_cond_broadcasts could just be signals.
     * But since error handling isn't a performance critical operation,
     * signals are used instead.
     */
    pthread_cond_broadcast(&sp->in_buf_readable_cond); /*multiple sp threads*/
    pthread_cond_broadcast(&sp->in_buf_done_cond);/* sp_write_input() caller */
    pthread_cond_broadcast(&sp->task_avail_cond);   /* multiple sp threads */
    pthread_cond_broadcast(&sp->task_drained_cond);/* sp_write_input() caller*/
    pthread_cond_broadcast(&sp->task_output_ready_cond); /* mult sp threads */
    pthread_cond_broadcast(&sp->task_output_empty_cond); /* mult sp threads */
    pthread_mutex_unlock(&sp->sump_mtx);
}


/* file_reader_test - main routine for a file reader thread using 
 *                            normal read() calls into an intermediate
 *                            buffer followed by writes into the sump pump.
 */
static void *file_reader_test(void *arg)
{
    size_t              size;
    char                *read_buf;
    sp_file_t           sp_src = (sp_file_t)arg;
    sp_t                sp = sp_src->sp;
    char                err_buf[200];
    
    TRACE("file_reader_intr: allocating %d buffer bytes\n",
          sp_src->transfer_size);
    read_buf = (char *)malloc(sp_src->transfer_size);
    
    /* keep looping until there is no additional input */
    for (;;)
    {
        size = read(sp_src->fd, read_buf, sp_src->transfer_size);
        if (size < 0)
        {
            sp_raise_error(sp, SP_FILE_READ_ERROR,
                           "%s: read() failure: %s\n",
                           sp_src->fname,
                           strerror_r(errno, err_buf, sizeof(err_buf)));
            break;
        }
        if (sp_write_input(sp, read_buf, size) != size)
            break;      /* silently quit on a downstream error */
        if (size == 0)  /* if we just sent 0 bytes to sp_write_input() */
            break;
    }
    TRACE("file_reader_test done: %d\n", sp_src->error_code);
    return (NULL);
}


/* file_reader_buffered - main routine for a file reader thread using normal
 *                        read() calls directly into sump pump input buffers.
 */
static void *file_reader_buffered(void *arg)
{
    size_t              size;
    size_t              request;
    u8                  index;
    char                *read_buf;
    int                 eof;
    sp_file_t           sp_src = (sp_file_t)arg;
    sp_t                sp = sp_src->sp;
    char                err_buf[200];
    
    /* keep looping until there is no additional input */
    for (index = 0; ; index++)
    {
        if (sp_get_in_buf(sp, index, (void **)&read_buf, &request) != SP_OK)
            break;
        size = read(sp_src->fd, read_buf, request);
        if (size < 0)
        {
            sp_raise_error(sp, SP_FILE_READ_ERROR,
                           "%s: read() failure: %s\n",
                           sp_src->fname,
                           strerror_r(errno, err_buf, sizeof(err_buf)));
            break;
        }
        eof = (size < request);
        if (sp_put_in_buf_bytes(sp, index, size, eof) != SP_OK)
            break;      /* silently quit on a downstream error */
        if (eof)
            break;
    }
    TRACE("file_reader_buffered done: %d\n", sp_src->error_code);
    return (NULL);
}


/* file_reader_direct - main routine for a file reader thread using direct
 *                      aio_read() calls on sump pump input buffers.
 */
static void *file_reader_direct(void *arg)
{
    ssize_t             size;
    ssize_t             request;
    ssize_t             in_buf_size;
    u8                  aios_started;
    off_t               file_read_offset = 0;
    struct sump_aio     *spaio;
    struct aiocb        *aio;
    const struct aiocb  *cb[1];
    int                 eof;
    sp_file_t           sp_src = (sp_file_t)arg;
    sp_t                sp = sp_src->sp;
    char                err_buf[200];
    int                 aio_count;
    int                 start;
    int                 done;
    int                 i;
    u8                  next_in_buf;
    size_t              next_buf_offset;
    char                *buf;
    int                 put_result;

    if (sp_src->aio_count <= 0)
        aio_count = 2;
    else
        aio_count = sp_src->aio_count;
    spaio = (struct sump_aio *)calloc(sizeof(struct sump_aio), aio_count);

    /* keep looping until there is no additional input */
    next_in_buf = 0;
    next_buf_offset = 0;
    for (aios_started = 0; ; aios_started++)
    {
        start = aios_started % aio_count;
        aio = &spaio[start].aio;
        aio->aio_fildes = sp_src->fd;
        /* if getting of the buffer fails. */
        if (sp_get_in_buf(sp, next_in_buf, &buf, &in_buf_size) != SP_OK)
        {
            sp_raise_error(sp, SP_FILE_READ_ERROR,
                           "sp_get_in_buf() failure with in_buf %lld\n",
                           aios_started);
            break;
        }
        request = in_buf_size - next_buf_offset;
        if (request > sp_src->transfer_size)
            request = sp_src->transfer_size;
        aio->aio_buf = buf + next_buf_offset;
        aio->aio_nbytes = request;
        aio->aio_offset = file_read_offset;
        spaio[start].buf_index = next_in_buf;
        spaio[start].buf_offset = next_buf_offset;
        spaio[start].file_offset = file_read_offset;
        spaio[start].nbytes = request;
        spaio[start].last_buf_io = (next_buf_offset + request == in_buf_size);
        next_buf_offset += request;
        if (next_buf_offset == in_buf_size)
        {
            next_in_buf++;
            next_buf_offset = 0;
        }
        if (aio_read(aio) < 0)
        {
            sp_src->error_code = SP_FILE_READ_ERROR;
            sp_raise_error(sp, SP_FILE_READ_ERROR,
                           "%s: aio_read() failure: %s, "
                           "offset: %lld, size: %lld\n",
                           sp_src->fname,
                           strerror_r(aio_error(&aio[start]), err_buf,
                                      sizeof(err_buf)),
                           file_read_offset, request);
            break;
        }
        file_read_offset += request;
        /* if the max number of aios has not been started, then start another.
         */
        if (aios_started < aio_count - 1)
            continue;

        /* wait for a previously issued request.
         */
        done = (aios_started + 1) % aio_count;
        aio = &spaio[done].aio;
        request = spaio[done].nbytes;
        cb[0] = aio;
        if (aio_suspend(cb, 1, NULL) != 0)
        {
            sp_src->error_code = SP_FILE_READ_ERROR;
            sp_raise_error(sp, SP_FILE_READ_ERROR,
                                                      "%s: aio_suspend() failure: %s, "
                           "offset: %lld, size: %lld\n",
                           sp_src->fname,
                           strerror_r(errno, err_buf, sizeof(err_buf)),
                           spaio[done].file_offset, request);
            break;
        }
        if ((size = aio_return(aio)) < 0)
        {
            sp_src->error_code = SP_FILE_READ_ERROR;
            sp_raise_error(sp, SP_FILE_READ_ERROR,
                           "%s: aio_return() failure: %s, "
                           "offset: %lld, size: %lld\n",
                           sp_src->fname,
                           strerror_r(aio_error(aio), err_buf,
                                      sizeof(err_buf)),
                           spaio[done].file_offset, request);
            break;
        }

        eof = (size < request);
        
        /* "put" the input buffer bytes if some bytes have been read into
         * the buffer and either eof or that was the last read for the buffer
         */
        size += spaio[done].buf_offset;
        put_result = SP_OK;
        if (size && (eof || spaio[done].last_buf_io))
        {
            put_result =
                sp_put_in_buf_bytes(sp, spaio[done].buf_index, size, eof);
        }
        if (put_result != SP_OK || eof)
        {
            /* clean up remaining aios and finish up */
            if (aio_count != 1)
            {
                /* wait for and ignore all previously issued aio's
                 */
                do
                {
                    done = (done + 1) % aio_count;
                    cb[0] = &spaio[done].aio;
                    aio_suspend(cb, 1, NULL);
                } while (done != start);
            }
            break;
        }
    }
    TRACE("file_reader_direct done: %d\n", sp_src->error_code);
    return (NULL);
}


/* file_writer_buffered - main routine for a file writer thread using normal
 *                        write() calls.
 */
static void *file_writer_buffered(void *arg)
{
    char                *buf;
    ssize_t             size;
    sp_file_t           sp_dst = (sp_file_t)arg;
    sp_t                sp = sp_dst->sp;
    int                 fd = sp_dst->fd;
    int                 out_index = sp_dst->out_index;
    char                err_buf[200];
    
    TRACE("file_writer_buffered: allocating %d buffer bytes\n", sp_dst->transfer_size);
    buf = (char *)malloc(sp_dst->transfer_size);
    
    for (;;)
    {
        size = sp_read_output(sp, out_index, buf, sp_dst->transfer_size);
        if (size <= 0)
        {
            if (size < 0)
            {
                sp_dst->error_code = SP_FILE_WRITE_ERROR;
            }
            break;
        }
        TRACE("writer: writing %d bytes\n", size);  
        if (write(fd, buf, size) != size)
        {
            sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                           "%s: write() failure: %s\n",
                           sp_dst->fname,
                           strerror_r(errno, err_buf, sizeof(err_buf)));
            sp_dst->error_code = SP_FILE_WRITE_ERROR;
            break;
        }
    }
    TRACE("file_writer_buffered done: %d\n", sp_dst->error_code);
    return (NULL);
}


/* file_writer_direct - main routine for a file writer thread using direct
 *                      aio_write() calls.
 */
static void *file_writer_direct(void *arg)
{
    char                *buf;
    ssize_t             size;
    sp_file_t           sp_dst = (sp_file_t)arg;
    sp_t                sp = sp_dst->sp;
    int                 out_index = sp_dst->out_index;
    ssize_t             request;
    u8                  aios_started;
    u8                  aios_completed;
    off_t               file_write_offset = 0;
    struct sump_aio     *spaio;
    struct aiocb        *aio;
    const struct aiocb  *cb[1];
    int                 aio_count;
    char                err_buf[200];
    int                 start;
    int                 done;
    int                 eof;
    int                 ret;
    char                *remainder_start = NULL;
    off_t               remainder_size = 0;

    if (sp_dst->aio_count <= 0)
        aio_count = 2;
    else
        aio_count = sp_dst->aio_count;
    spaio = (struct sump_aio *)calloc(sizeof(struct sump_aio), aio_count);

    TRACE("file_writer_direct: allocating %d buffer bytes\n", sp_dst->transfer_size);
    size = sp_dst->transfer_size * aio_count;
    init_zero_fd();
    buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, Zero_fd, 0);
    if (buf == MAP_FAILED)
    {
        sp_raise_error(sp, SP_MEM_ALLOC_ERROR,
                       "mmap() failure: %s\n",
                       strerror_r(errno, err_buf, sizeof(err_buf)));
        sp_dst->error_code = SP_MEM_ALLOC_ERROR;
        return;
    }

    aios_completed = 0;
    for (aios_started = 0; ; )
    {
        start = aios_started % aio_count;
        aio = &spaio[start].aio;
        aio->aio_fildes = sp_dst->fd;
        aio->aio_buf = buf + start * sp_dst->transfer_size;
        request = sp_read_output(sp, out_index, (void *)aio->aio_buf,
                              sp_dst->transfer_size);
        if (request < 0)
        {
            sp_dst->error_code = SP_FILE_WRITE_ERROR;
            break;
        }

        /* if not a write or not a full write, then its eof.
         */
        eof = (request < sp_dst->transfer_size);
        if (eof &&
            (remainder_size = request % Page_size) != 0)
        {
            request -= remainder_size;
            remainder_start = (char *)aio->aio_buf + request;
        }
        if (request != 0)
        {
            TRACE("file_writer_direct: writing %d bytes\n", request);  
            aio->aio_nbytes = request;
            aio->aio_offset = file_write_offset;
            spaio[start].buf_index = 0;    /* not used */
            spaio[start].buf_offset = 0;   /* not used */
            spaio[start].file_offset = file_write_offset;
            spaio[start].nbytes = request;
            spaio[start].last_buf_io = 0;  /* not used */
            if (aio_write(aio) < 0)
            {
                sp_dst->error_code = SP_FILE_WRITE_ERROR;
                sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                               "%s: aio_write() failure: %s, "
                               "offset: %lld, size: %lld\n",
                               sp_dst->fname,
                               strerror_r(aio_error(&aio[start]), err_buf,
                                          sizeof(err_buf)),
                               spaio[done].file_offset, request);
                break;
            }
            file_write_offset += request;
            aios_started++;
        }
        else if (aios_started == 0)  /* if no direct output whatsoever */
            break;
        
        /* if not eof and the max number of aios has not been started,
         * then start another.
         */
        if (!eof && aios_started < aio_count)
            continue;

        /* wait for a previously issued request.
         * if eof, then wait for all previously issued requests.
         */
        do
        {
            done = aios_completed % aio_count;
            aio = &spaio[done].aio;
            request = spaio[done].nbytes;
            cb[0] = aio;
            while ((ret = aio_suspend(cb, 1, NULL)) != 0 && errno == EINTR)
                continue;
            if (ret != 0)
            {
                sp_dst->error_code = SP_FILE_WRITE_ERROR;
                sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                               "%s: aio_suspend() failure: %s, "
                               "offset: %lld, size: %lld\n",
                               sp_dst->fname,
                               strerror_r(errno, err_buf, sizeof(err_buf)),
                               spaio[done].file_offset, request);
                break;
            }
            if ((size = aio_return(aio)) < 0)
            {
                sp_dst->error_code = SP_FILE_WRITE_ERROR;
                sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                               "%s: aio_return() failure: %s, "
                               "offset: %lld, size: %lld\n",
                               sp_dst->fname,
                               strerror_r(aio_error(aio), err_buf,
                                          sizeof(err_buf)),
                               spaio[done].file_offset, request);
                break;
            }
            if (size != request)
            {
                sp_dst->error_code = SP_FILE_WRITE_ERROR;
                sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                               "%s: aio_write() return failure: %s, "
                               "offset: %lld, "
                               "returned size %lld != requested size: %lld\n",
                               sp_dst->fname,
                               strerror_r(aio_error(aio), err_buf,
                                          sizeof(err_buf)),
                               spaio[done].file_offset, size, request);
                break;
            }
            aios_completed++;
        } while (eof && aios_completed < aios_started);
        if (eof || sp_dst->error_code != 0)
            break;
    }
    if (sp_dst->error_code == 0 && remainder_size != 0)
    {
        int     fd;

        /* close file descriptor opened with O_DIRECT */
        close(sp_dst->fd);
        /* reopen file without O_DIRECT */
        if ((fd = open(sp_dst->fname, O_WRONLY, 0777)) < 0)
        {
            sp_dst->error_code = SP_FILE_WRITE_ERROR;
            sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                           "%s: remainder open() return failure: %s\n",
                           sp_dst->fname,
                           strerror_r(errno, err_buf, sizeof(err_buf)));
        }
        else
        {
            ret = pwrite(fd, remainder_start,
                         remainder_size, file_write_offset);
            if (ret != remainder_size)
            {
                sp_dst->error_code = SP_FILE_WRITE_ERROR;
                sp_raise_error(sp, SP_FILE_WRITE_ERROR,
                               "%s: pwrite() return failure: %s, "
                               "offset: %lld, "
                               "returned size %lld != requested size: %lld\n",
                               sp_dst->fname,
                               strerror_r(errno, err_buf, sizeof(err_buf)),
                               file_write_offset, ret, remainder_size);
            }
        }
    }
    TRACE("file_writer_direct done: %d\n", sp_dst->error_code);
    return;
}


/* sp_wait - can be called by an external thread, e.g. the thread that
 *           called sp_start(), to wait for all sump pump activity to cease.
 */
int sp_wait(sp_t sp)
{
    int         i;
    int         ret;

    if (sp->flags & SP_SORT)
    {
        pthread_mutex_lock(&sp->sump_mtx);
        while (sp->error_code == 0 && sp->sort_state != SORT_DONE)
        {
            TRACE("sp_wait() condition wait");
            pthread_cond_wait(&sp->task_output_ready_cond, &sp->sump_mtx);
        }
        pthread_mutex_unlock(&sp->sump_mtx);
        return (sp->error_code);
    }
    else /* non-sort sump pump */
    {
        if (sp->in_file_sp != NULL)
        {
            if ((ret = sp_file_wait(sp->in_file_sp)) != SP_OK)
                return (sp->error_code == SP_OK ? ret : sp->error_code);
        }
        for (i = 0; i < sp->num_threads; i++)
            pthread_join(sp->thread[i], NULL);
        for (i = 0; i < sp->num_outputs; i++)
        {
            if (sp->out[i].file_sp != NULL)
            {
                if ((ret = sp_file_wait(sp->out[i].file_sp)) != SP_OK)
                    return (sp->error_code == SP_OK ? ret : sp->error_code);
            }
        }
    }
    return (sp->error_code);
}


/* syntax error - internal routine to print out an nsort error message
 */
static void syntax_error(sp_t sp, char *p, char *err_msg)
{
    int         ret;

    if (sp->error_code != 0)          /* if prior error */
        return;                       /* ignore this one */
    sp->error_code = SP_SYNTAX_ERROR;

    ret = snprintf(sp->error_buf, sp->error_buf_size,
                   "syntax error: %s at: \"%s\"\n", err_msg, p);
    if (ret >= sp->error_buf_size)
    {
        if (sp->error_buf_size != 0)
            free(sp->error_buf);
        sp->error_buf_size = ret + 1;
        sp->error_buf = (char *)malloc(sp->error_buf_size);
        snprintf(sp->error_buf, sp->error_buf_size,
                 "syntax error: %s at: \"%s\"\n", err_msg, p);
    }
    
    return;
}


/* get_numeric_arg - internal routine to convert an ascii number to i8
 */
static i8 get_numeric_arg(sp_t sp, char **caller_p)
{
    int         negative = 0;
    i8          result = 0;
    char        *p = *caller_p;

    if (*p == '-')
    {
        p++;
        negative = 1;
    }
    if (*p < '0' || *p > '9')
    {
        syntax_error(sp, p, "expected numeric argument");
        return 0;
    }
    while (*p >= '0' && *p <= '9')
    {
        result = result * 10 + (*p - '0');
        p++;
    }
    *caller_p = p;
    return (negative ? -result : result);
}


/* get_scale - internal routine to convert 'k', 'm' of 'g' to a numeric value
 */
static i8 get_scale(char **caller_p)
{
    char        *p = *caller_p;
    i8          factor = 1;

    if (*p == 'k' || *p == 'K')
        factor = 1024;
    else if (*p == 'm' || *p == 'M')
        factor = 1024 * 1024;
    else if (*p == 'g' || *p == 'G')
        factor = 1024 * 1024 * 1024;
    else
        return (1);
    *caller_p = p + 1;
    return (factor);
}


/* get_file_mods - get file name modifiers, e.g. access mode and transfer size.
 */
void get_file_mods(sp_file_t spf, char *mods)
{
    struct stat statbuf;
    char        *p = mods;
    char        *kw;

    for (;;)
    {
        while (*p == ',')
            p++;
        if (*p == '\0')
            break;
        else if (kw = "BUFFERED", !strncasecmp(p, kw, strlen(kw)))
        {
            p += strlen(kw);
            spf->mode = MODE_BUFFERED;
        }
        else if (kw = "DIRECT", !strncasecmp(p, kw, strlen(kw)))
        {
            p += strlen(kw);
            spf->mode = MODE_DIRECT;
        }
        else if (kw = "COUNT", !strncasecmp(p, kw, strlen(kw)))
        {
            p += strlen(kw);
            if (*p != ':' && *p != '=')
            {
                syntax_error(spf->sp, p, "expected ':' or '=' after 'count'");
                return;
            }
            p++;
            spf->aio_count = get_numeric_arg(spf->sp, &p);
        }
        else if ((kw = "TRANSFER", !strncasecmp(p, kw, strlen(kw))) ||
                 (kw = "TRANS", !strncasecmp(p, kw, strlen(kw))))
        {
            p += strlen(kw);
            if (*p != ':' && *p != '=')
            {
                syntax_error(spf->sp, p, "expected ':' of '='");
                return;
            }
            p++;
            spf->transfer_size = get_numeric_arg(spf->sp, &p);
            spf->transfer_size *= get_scale(&p);
        }
        else
        {
            syntax_error(spf->sp, p, "unrecognized file modifier");
            break;
        }
    }
}


/* sp_open_file_src - use the specified file as the input for the
 *                    specified sump pump.
 */
sp_file_t sp_open_file_src(sp_t sp, const char *fname_mods, unsigned flags)
{
    sp_file_t   sp_src;
    char        *comma_char;
    int         fname_len;
    void        *(*reader_main)(void *);

    if ((sp_src = (sp_file_t)calloc(1, sizeof(struct sp_file))) == NULL)
        return (NULL);
    sp_src->sp = sp;

    comma_char = strchr(fname_mods, ',');
    fname_len =
        comma_char == NULL ? strlen(fname_mods) : comma_char - fname_mods;
    sp_src->fname = (char *)calloc(1, fname_len + 1);
    memcpy(sp_src->fname, fname_mods, fname_len);
    sp_src->fname[fname_len] = '\0';
    if (comma_char != NULL)
        get_file_mods(sp_src, comma_char + 1);
    if (sp_src->mode == MODE_UNSPECIFIED)
        sp_src->mode = Default_file_mode;
    if (sp_src->mode == MODE_DIRECT && strcmp(sp_src->fname, "<stdin>") != 0)
    {
        reader_main = file_reader_direct;
        flags |= O_DIRECT;
        if (sp_src->transfer_size == 0)
            sp_src->transfer_size = 512 * 1024;
        if (sp_src->aio_count == 0)
            sp_src->aio_count = 4;
    }
    else
    {
        if (Default_rw_test_size != 0)
        {
            reader_main = file_reader_test;
            sp_src->transfer_size = Default_rw_test_size;
        }
        else
            reader_main = file_reader_buffered;
    }

    if (strcmp(sp_src->fname, "<stdin>") == 0)
        sp_src->fd = 0;
    else
    {
        sp_src->fd = open(sp_src->fname, flags);
        if (sp_src->fd < 0)
            return (NULL);
    }
    
    /* create reader thread */
    if (pthread_create(&sp_src->thread, NULL, reader_main, sp_src) != 0)
        return (NULL);
    return (sp_src);
}


/* sp_open_file_dst - use the specified file as the output for the
 *                    specified output of the specified sump pump.
 */
sp_file_t sp_open_file_dst(sp_t sp, unsigned out_index, const char *fname_mods, unsigned flags)
{
    sp_file_t           sp_dst;
    int                 ret;
    char                *comma_char;
    int                 fname_len;
    void                *(*writer_main)(void *);

    if ((sp_dst = (sp_file_t)calloc(1, sizeof(struct sp_file))) == NULL)
        return (NULL);
    sp_dst->sp = sp;

    comma_char = strchr(fname_mods, ',');
    fname_len =
        comma_char == NULL ? strlen(fname_mods) : comma_char - fname_mods;
    sp_dst->fname = (char *)calloc(1, fname_len + 1);
    memcpy(sp_dst->fname, fname_mods, fname_len);
    sp_dst->fname[fname_len] = '\0';
    if (comma_char != NULL)
        get_file_mods(sp_dst, comma_char + 1);
    if (sp_dst->mode == MODE_UNSPECIFIED)
        sp_dst->mode = Default_file_mode;
    if (sp_dst->mode == MODE_DIRECT &&
        strcmp(sp_dst->fname, "<stdout>") != 0 &&
        strcmp(sp_dst->fname, "<stderr>") != 0)
    {
        writer_main = file_writer_direct;
        flags |= O_DIRECT;
        if (sp_dst->transfer_size == 0)
            sp_dst->transfer_size = 512 * 1024;
        if (sp_dst->aio_count == 0)
            sp_dst->aio_count = 4;
    }
    else
    {
        writer_main = file_writer_buffered;
        if (Default_rw_test_size != 0)
            sp_dst->transfer_size = Default_rw_test_size;
    }

    if (strcmp(sp_dst->fname, "<stdout>") == 0)
        sp_dst->fd = 1;
    else if (strcmp(sp_dst->fname, "<stderr>") == 0)
        sp_dst->fd = 2;
    else
    {
        sp_dst->fd = open(sp_dst->fname, flags, 0777);
        if (sp_dst->fd < 0)
            return (NULL);
    }
    sp_dst->out_index = out_index;
    if (sp_dst->transfer_size == 0)
        sp_dst->transfer_size =
            Default_rw_test_size ? Default_rw_test_size : 4096;
    /* create writer thread */
    if ((ret = pthread_create(&sp_dst->thread, NULL, writer_main, sp_dst)))
        die("sp_open_file_dst: pthread_create() ret: %d\n", ret);
    return (sp_dst);
}


/* sp_file_wait - wait for the specified file connection to complete.
 */
int sp_file_wait(sp_file_t sp_file)
{
    pthread_join(sp_file->thread, NULL);
    return (sp_file->error_code);
}


/* check_task_done - internal routine to make sure there is room for at
 *                   least one new task.  
 *                   Caller must have locked sump_mtx.
 */
static void check_task_done(sp_t sp)
{
    sp_task_t   t;

    TRACE("check_task_done() called\n");

    /* while there are tasks which have not yet been recognized as done.
     */
    if (sp->cnt_task_init > sp->cnt_task_done)
    {
        /* while there is no room for a new task
         */
        while (sp->error_code == 0 &&
               (sp->cnt_task_init >
                sp->cnt_task_drained + sp->num_tasks - 1))
        {
            TRACE("check_task_done() condition wait for task %d\n",
                  sp->cnt_task_drained);
            pthread_cond_wait(&sp->task_drained_cond, &sp->sump_mtx);
        }
        if (sp->error_code != 0)
        {
            TRACE("check_task_done() returning because of error_code: %d\n",
                  sp->error_code);
            return;
        }

        /* Verify the actual ending position of each done task
         * matches its expected ending position.
         */
        while (sp->cnt_task_drained > sp->cnt_task_done)
        {
            t = &sp->task[sp->cnt_task_done % sp->num_tasks];
            TRACE("check_task_done() task %d verify\n",
                  sp->cnt_task_done);
            if (t->curr_in_buf_index != t->expected_end_index ||
                (t->curr_rec - t->in_buf) != t->expected_end_offset)
            {
                die("task %d input ending mismatch: "
                    "ci %d, ei %d, ao %d, eo %d\n",
                    sp->cnt_task_done,
                    t->curr_in_buf_index,
                    t->expected_end_index,
                    (t->curr_rec - t->in_buf),
                    t->expected_end_offset);
            }
            sp->cnt_task_done++;
        }
    }
    TRACE("check_task_done() returning\n");
}


/* post_nsort_error - internal routine to post an error received from nsort.
 */
static void post_nsort_error(sp_t sp, unsigned ret)
{
    pthread_mutex_lock(&sp->sump_mtx);
    if (sp->error_code == SP_OK)  /* if no other error yet, this is the one */
    {
        char *msg = (*Nsort_message)(&sp->nsort_ctx);
        
        sp->error_code = SP_SORT_EXEC_ERROR;
        sp->sort_error = ret;
        if (msg == NULL)
            msg = "No Nsort error message";
        strncpy(sp->error_buf, msg, sp->error_buf_size);
        sp->error_buf[sp->error_buf_size - 1] = '\0';  /* handle overflow */
    }
    sp->sort_error = ret;
    sp->sort_state = SORT_DONE;
    pthread_cond_broadcast(&sp->task_output_ready_cond);
    pthread_mutex_unlock(&sp->sump_mtx);
}


/* init_new_task - internal routine called by sp_write_input() to
 *                 initialize a new sump pump task.
 */
static sp_task_t init_new_task(sp_t sp, in_buf_t *ib, char *curr_rec)
{            
    sp_task_t           t;
    int                 i;

    t = &sp->task[sp->cnt_task_init % sp->num_tasks];
    t->task_number = sp->cnt_task_init;
    /* record starting ib and offset */
    t->in_buf = ib->in_buf;
    /* if the size is 0, this empty task indicates EOF */
    t->in_buf_bytes = ib->in_buf_bytes;
    t->curr_rec = curr_rec;
    t->begin_rec = t->curr_rec;
    t->curr_in_buf_index = sp->cnt_in_buf_readable - 1;
    t->begin_in_buf_index = t->curr_in_buf_index;
    t->expected_end_index = -1;
    t->expected_end_offset = -1;
    t->first_in_buf = TRUE;
    t->outs_drained = 0;
    for (i = 0; i < sp->num_outputs; i++)
    {
        t->out[i].bytes_copied = 0;
        t->out[i].stalled = FALSE;
    }
    t->input_eof = FALSE;
    t->output_eof = FALSE;
    sp->cnt_task_init++;
}


/* new_in_buf - wait, if necessary, until an in_buf is available to be filled
 *              with input data.
 */
static void new_in_buf(sp_t sp)
{
    in_buf_t            *ib = NULL;

    TRACE("new_in_buf: waiting for buffer\n"); 
    pthread_mutex_lock(&sp->sump_mtx);
    while (sp->error_code == 0 &&
           sp->cnt_in_buf_readable >= sp->cnt_in_buf_done + sp->num_in_bufs)
    {
        /* get oldest buffer not yet recognized as done */
        ib = &sp->in_buf[sp->cnt_in_buf_done % sp->num_in_bufs];
        /* if all readers of this input buffer are done reading */
        if (ib->num_readers == ib->num_readers_done)
        {
            sp->cnt_in_buf_done++;
            break;
        }
        pthread_cond_wait(&sp->in_buf_done_cond, &sp->sump_mtx);
    }
    pthread_mutex_unlock(&sp->sump_mtx);
}


/* eof_without_new_in_buf_or_task - clean eof, no need to flush an input
 *                                  buffer or start a new task.
 */
static void eof_without_new_in_buf_or_task(sp_t sp)
{
    pthread_mutex_lock(&sp->sump_mtx);
    sp->input_eof = TRUE;
    /* wake sump thread waiting for next input buffer (just signal OK?) */
    pthread_cond_broadcast(&sp->in_buf_readable_cond);
    /* wake all sump threads waiting for new task */
    pthread_cond_broadcast(&sp->task_avail_cond);
    /* wake writer thread as it should exit on EOF */
    pthread_cond_broadcast(&sp->task_output_ready_cond); 
    pthread_mutex_unlock(&sp->sump_mtx);
}


/* flush_in_buf - flush an input buffer and start a new task if necessary
 */
static void flush_in_buf(sp_t sp, size_t buf_bytes, int eof)
{
    in_buf_t            *ib;
    sp_task_t           t;
    char                *curr_rec;
    char                *p;
    int                 i;
    int                 key_offset;
        
    /* get ready to release in_buf to member threads executing pump funcs */
    /* initially, no readers (there will be at least one) */
    ib = &sp->in_buf[sp->cnt_in_buf_readable % sp->num_in_bufs];
    ib->num_readers = 0;     
    ib->num_readers_done = 0;
    curr_rec = ib->in_buf;
    ib->in_buf_bytes = buf_bytes;
    sp->in_buf_current_bytes = 0;

    /* if this is not the first buffer and we are not processing
     * whole buffers.
     */
    if (sp->cnt_in_buf_readable != 0 && REC_TYPE(sp) != SP_WHOLE_BUF)
    {
        /* if we are grouping record by key values
         */
        if (sp->flags & SP_REDUCE_BY_KEYS)
        {
            /* the member thread performing the most previously
             * issued task will always read this input buffer in
             * order to find the end of its input
             */
            ib->num_readers = 1;

            switch (REC_TYPE(sp))
            {
              case SP_UTF_8:
                /* scan until either we find a record whose key
                 * difference index is less than the number of
                 * "group by" keys, or we scan to the end of the buffer.
                 */
                while (curr_rec < ib->in_buf + ib->in_buf_bytes)
                {
                    /* if this is not the beginning of the buffer or
                     * the buffer begins with a whole record.
                     */
                    if (curr_rec != ib->in_buf ||
                        sp->prev_in_buf_ending_rec_partial_bytes == 0)
                    {
                        /* if key offset indicates a new key
                         * grouping, then stop as this the boundry
                         * point between tasks.
                         */
                        key_offset = *curr_rec - '0';
                        if (key_offset < sp->group_by_keys)
                            break;
                    }
                    /* find next instance of newline in buffer, if any */
                    curr_rec = memchr(curr_rec, *(char *)sp->delimiter,
                                      ib->in_buf_bytes -
                                      (curr_rec - ib->in_buf));
                    if (curr_rec == NULL)
                    {
                        curr_rec = ib->in_buf + ib->in_buf_bytes;
                        break;
                    }
                    curr_rec++;     /* step over newline */
                }
                break;

              case SP_FIXED:
                if (sp->prev_in_buf_ending_rec_partial_bytes == 0)
                    curr_rec = ib->in_buf;
                else
                    curr_rec = ib->in_buf + sp->rec_size -
                        sp->prev_in_buf_ending_rec_partial_bytes;
                while (curr_rec < ib->in_buf + ib->in_buf_bytes)
                {
                    key_offset = *curr_rec - '0';
                    if (key_offset < sp->group_by_keys)
                        break;
                    curr_rec += sp->rec_size;
                }
                break;
            }
        }
        else   /* we are not grouping records by key value */
        {
            /* if this in buffer starts with a partial record, then
             * the member thread performing the previous issued task
             * will read this buffer in order to find the end of its
             * input
             */
            if (sp->prev_in_buf_ending_rec_partial_bytes != 0)
            {
                /* the previously issued task is required to read the
                 * record reamainder at the beginning of this buffer.
                 */
                ib->num_readers = 1;

                switch (REC_TYPE(sp))
                {
                  case SP_UTF_8:
                    /* find next instance of newline in buffer, if any */
                    curr_rec = memchr(curr_rec, *(char *)sp->delimiter,
                                      ib->in_buf_bytes -
                                      (curr_rec - ib->in_buf));
                    if (curr_rec == NULL)
                        curr_rec = ib->in_buf + ib->in_buf_bytes;
                    else
                        curr_rec++;     /* step over newline */
                    break;

                  case SP_FIXED:
                    curr_rec = ib->in_buf + (sp->rec_size -
                                             sp->prev_in_buf_ending_rec_partial_bytes);
                    break;
                }            
            }
        }
    }

    switch (REC_TYPE(sp))
    {
      case SP_UTF_8:
        /* determine how many bytes the are in any partial record at
         * the end of this buffer.  search backwards to find last newline
         * character.
         */
        for (p = ib->in_buf + ib->in_buf_bytes - 1; p >= ib->in_buf; p--)
            if (*p == *(char *)sp->delimiter) 
                break;
        if (p < ib->in_buf)      /* if there was no newline */
        {
            /* add entire buffer size to this partial record */
            sp->prev_in_buf_ending_rec_partial_bytes += ib->in_buf_bytes;
        }
        else
            sp->prev_in_buf_ending_rec_partial_bytes =
                (ib->in_buf + ib->in_buf_bytes - 1) - p;
        break;

      case SP_FIXED:
        sp->prev_in_buf_ending_rec_partial_bytes =
            (sp->prev_in_buf_ending_rec_partial_bytes + ib->in_buf_bytes) %
            sp->rec_size;
        break;

      case SP_WHOLE_BUF:
        sp->prev_in_buf_ending_rec_partial_bytes = 0;
        curr_rec = ib->in_buf;
        break;
    }
        
    TRACE("flush_in_buf: ib %d readable with %d bytes\n",
          sp->cnt_in_buf_readable, ib->in_buf_bytes);
    pthread_mutex_lock(&sp->sump_mtx);
    /* make input buffer available to any existing task.
     * in theory there should be only one task waiting */
    sp->cnt_in_buf_readable++;
    pthread_cond_broadcast(&sp->in_buf_readable_cond);

    if (eof)
    {
        /* if we found the starting point for a new task, then add a
         * reader for the task that will start its input with this
         * input buffer.
         */
        if (curr_rec < ib->in_buf + ib->in_buf_bytes)
            ib->num_readers++;

        /* set the expected end point for the previous task as the
         * actual begin point for the task we are about to define.
         */
        if (sp->cnt_task_init != 0) /* if there was a previous task */
        {
            t = &sp->task[(sp->cnt_task_init - 1) % sp->num_tasks];

            /* if we just made availible a non-empty in_buf
             * that did NOT start a new task.
             */
            if (ib->in_buf_bytes != 0 &&
                curr_rec >= ib->in_buf + ib->in_buf_bytes)
            {
                /* the expected ending in_buf is the next (and empty)
                 * one to be issued.
                 */
                t->expected_end_index = sp->cnt_in_buf_readable;
                t->expected_end_offset = 0;
                pthread_mutex_unlock(&sp->sump_mtx);
                /* do not issue a new task here. */
                eof_without_new_in_buf_or_task(sp);
                return;
            }
            else
            {
                /* the expected ending in_buf is the one just issued
                 * and the offset is the end.
                 */
                t->expected_end_index = sp->cnt_in_buf_readable - 1;
                t->expected_end_offset = curr_rec - ib->in_buf;
            }

            /* Note: it is possible that at this point the previous
             * task has already completed.  This is why tasks should
             * not use their expected ending point to confirm that
             * they ended at the right point.  This thread should
             * perform the check after the task has completed. */
        }

        /* make sure there is at least one available task struct */
        check_task_done(sp);
        if (sp->error_code != 0)
        {
            pthread_mutex_unlock(&sp->sump_mtx);
            return;
        }

        TRACE("flush_in_buf: initializing task %d\n", sp->cnt_task_init);
        t = init_new_task(sp, ib, curr_rec);
        sp->input_eof = TRUE;
        /* wake all sump threads */
        pthread_cond_broadcast(&sp->task_avail_cond);
        /* wake writer thread as it should exit on EOF */
        pthread_cond_broadcast(&sp->task_output_ready_cond); 
        pthread_mutex_unlock(&sp->sump_mtx);
        return;
    }

    /* if we found the ending point for a task (or this was the
     * very first input read), then start a new task.
     */
    if (curr_rec < ib->in_buf + ib->in_buf_bytes)
    {
        /* add a reader for the task that will start its input with
         * this input buffer.
         */
        ib->num_readers++;

        /* set the expected end point for the previous task as the
         * actual begin point for the task we are about to define.
         */
        if (sp->cnt_task_init != 0) /* if there was a previous task */
        {
            t = &sp->task[(sp->cnt_task_init - 1) % sp->num_tasks];

            /* the expected ending in_buf is the one just issued
             * and the offset is.
             */
            t->expected_end_index = sp->cnt_in_buf_readable - 1;
            t->expected_end_offset = curr_rec - ib->in_buf;

            /* Note: it is possible that at this point the previous
             * task has already completed.  This is why tasks should
             * not use their expected ending point to confirm that
             * they ended at the right point.  This thread should
             * perform the check after the task has completed. */
        }

        /* make sure there is at least one available task struct */
        check_task_done(sp);
        if (sp->error_code != 0)
        {
            pthread_mutex_unlock(&sp->sump_mtx);
            return;
        }

        TRACE("flush_in_buf: initializing task %d\n", sp->cnt_task_init);
        t = init_new_task(sp, ib, curr_rec);

        /* wake 1 sump thread */
        pthread_cond_signal(&sp->task_avail_cond);
    }
    pthread_mutex_unlock(&sp->sump_mtx);
    return;
}


/* sp_write_input - write data that is the input to a sump pump.
 *                  A write size of 0 indicates input EOF.
 */
ssize_t sp_write_input(sp_t sp, void *buf, ssize_t size)
{
    size_t              src_remaining = size;
    size_t              dst_remaining;
    size_t              trans_size;
    char                *trans_src;
    char                *trans_dst;
    in_buf_t            *ib;
    int                 ret;
            
    TRACE("sp_write_input: size %d\n", size);
    
    if (size <= 0 && sp->input_eof)
        return (0);    /* ignore, already eof */
    if (sp->error_code)
        return (-1);   /* already error */

    if (size <= 0)
    {
        if (size < 0)
        {
            /* TO-DO: need to make sure a partial input record does not
             * cause a sump hang instead of a sump error.  A sump hang
             * shouldn't matter if sump pump invoker waits for sump
             * pumps in upstream-to-downstream order */
            sp->error_code = SP_UPSTREAM_ERROR;
            size = 0;   /* act as if normal eof */
            if (sp->flags & SP_SORT)
            {
                (*Nsort_end)(&sp->nsort_ctx);
            }
            return (0);  /* caller is indicating error, return OK status */
        }
        /* else size == 0 */
        if (sp->flags & SP_SORT)
        {
            ret = (*Nsort_release_end)(&sp->nsort_ctx);

            if (ret < 0)        /* if error */
            {
                post_nsort_error(sp, ret);
                return (-1);
            }
            else
            {
                pthread_mutex_lock(&sp->sump_mtx);
                sp->sort_state = SORT_OUTPUT;
                pthread_cond_broadcast(&sp->task_output_ready_cond);
                pthread_mutex_unlock(&sp->sump_mtx);
                return (0);
            }
        }
        /* for non-sort case, fall through */
    }

    if (sp->flags & SP_SORT)
    {
        if (sp->sort_state != SORT_INPUT)
            return (0);
        ret = (*Nsort_release_recs)(buf, size, &sp->nsort_ctx);
        if (ret < 0)        /* if error */
            post_nsort_error(sp, ret);
        return (ret == NSORT_SUCCESS ? size : 0);
    }

    /* if EOF and there isn't a partially filled input buffer needing release.
     */
    if (src_remaining == 0 && sp->in_buf_current_bytes == 0)
    {
        eof_without_new_in_buf_or_task(sp);
        return (0);
    }
    
    /* while this is not EOF or there is a partially filled input buffer
     */
    while (src_remaining != 0 || sp->in_buf_current_bytes != 0)
    {
        /* if it is NOT the case that we have already partially filled an
         * input buffer that has not yet been released to the sump pump.
         * then get a new input buffer.
         */
        if (sp->in_buf_current_bytes == 0)
        {
            new_in_buf(sp);
            if (sp->error_code != 0)
                return (-1);
        }

        ib = &sp->in_buf[sp->cnt_in_buf_readable % sp->num_in_bufs];
        TRACE("sp_write_input: readable: %d, partial: %d\n",
              sp->cnt_in_buf_readable, sp->in_buf_current_bytes);

        trans_src = (char *)buf + size - src_remaining;
        trans_size = src_remaining;
        trans_dst = ib->in_buf + sp->in_buf_current_bytes;
        dst_remaining = ib->in_buf_size - sp->in_buf_current_bytes;
        if (trans_size > dst_remaining)
            trans_size = dst_remaining;
        memcpy(trans_dst, trans_src, trans_size);
        src_remaining -= trans_size;
        sp->in_buf_current_bytes += trans_size;
        dst_remaining = ib->in_buf_size - sp->in_buf_current_bytes;

        /* if this isn't EOF and there is more space remaining the in_buf,
         * then return so caller can write more bytes or declare EOF.
         */
        if (size != 0 && dst_remaining > 0)
        {
            if (src_remaining != 0)
                die("sp_write_input: dst_remaining %d and src_remaining %d\n",
                    dst_remaining, src_remaining);
            TRACE("sp_write_input() returning %d, dst_remaining: %d\n",
                  size, dst_remaining);
            return size;
        }

        flush_in_buf(sp, sp->in_buf_current_bytes, size == 0);
    }

    if (sp->error_code)
        size = -1;
    TRACE("sp_write_input: returning %d\n", size);
    return (size);
}


/* sp_get_in_buf - get a pointer to an input buffer that an external
 *                    thread can fill with input data.
 */
int sp_get_in_buf(sp_t sp, u8 buf_index, void **buf, size_t *size)
{
    in_buf_t    *ib;
    
    if (sp->flags & SP_SORT)
        return (SP_SORT_INCOMPATIBLE);
    *buf = NULL;
    *size = 0;
    TRACE("sp_get_in_buf: waiting for input buffer\n");
    if (buf_index < sp->cnt_in_buf_readable ||
        buf_index >= sp->cnt_in_buf_readable + sp->num_in_bufs)
    {
        return (SP_BUF_INDEX_ERROR);
    }
    pthread_mutex_lock(&sp->sump_mtx);
    while (sp->error_code == 0 &&
           buf_index >= sp->cnt_in_buf_done + sp->num_in_bufs)
    {
        /* get oldest buffer not yet recognized as done */
        ib = &sp->in_buf[sp->cnt_in_buf_done % sp->num_in_bufs];
        /* if all readers of this input buffer are done reading */
        if (ib->num_readers == ib->num_readers_done)
        {
            sp->cnt_in_buf_done++;
            continue;
        }
        pthread_cond_wait(&sp->in_buf_done_cond, &sp->sump_mtx);
    }
    if (sp->error_code == 0)
    {
        ib = &sp->in_buf[buf_index % sp->num_in_bufs];
        *buf = ib->in_buf;
        *size = ib->in_buf_size;
    }
    pthread_mutex_unlock(&sp->sump_mtx);
    if (sp->error_code)
        return (sp->error_code);
    return (SP_OK);
}


/* sp_put_in_buf_bytes - flush bytes that have been placed in a sump
 *                          pump's input buffer by an external thread.
 *                          This function should only be used by first
 *                          calling sp_get_in_buf().
 */
int sp_put_in_buf_bytes(sp_t sp, u8 buf_index, size_t size, int eof)
{
    if (sp->flags & SP_SORT)
        return (SP_SORT_INCOMPATIBLE);
    if (buf_index != sp->cnt_in_buf_readable)
        return (SP_BUF_INDEX_ERROR);
    if (size == 0)
        eof_without_new_in_buf_or_task(sp);
    else
        flush_in_buf(sp, size, eof);
    if (sp->error_code)
        return (sp->error_code);
    return (SP_OK);
}


/* sp_get_error - get the error code of a sump pump.
 */
int sp_get_error(sp_t sp)
{
    return (sp->error_code);
}


/* pfunc_get_thread_index - can be used by pump functions to get the
 *                          index of the sump pump thread executing the
 *                          pump func.  For instance, if there are 4
 *                          sump pump threads, this function will return
 *                          0-3 depending on which of the 4 threads is
 *                          invoking it.
 */
int pfunc_get_thread_index(sp_task_t t)
{
    return (t->thread_index);
}


/* pfunc_get_thread_number - can be used by pump functions to get the sump
 *                           pump task number being executed by the pump
 *                           function.  This number starts at 0 and
 *                           increases with each subsequent task
 *                           issued/started by the sump pump.
 */
u8 pfunc_get_task_number(sp_task_t t)
{
    return (t->task_number);
}


/* pfunc_write - write function that can be used by a pump function to
 *               write the output data for the pump function.
 */
size_t pfunc_write(sp_task_t t, unsigned out_index, void *buf, size_t size)
{
    char        *src = (char *)buf;
    size_t      bytes_left = size;
    size_t      copy_bytes;
    sp_t        sp = t->sp;
    struct task_out *out = t->out + out_index;

    if (sp->error_code != SP_OK)
        return (0);
    
    if (out_index >= sp->num_outputs)
    {
        pfunc_error(t, "pfunc_write, out_index %d >= num_outputs %d\n",
                    out_index, sp->num_outputs);
        sp->error_code = SP_OUTPUT_INDEX_ERROR;
        return (0);
    }
    
    /* while can't fit existing map output buffer contents and the new record
     * into the map output buffer.
     */
    while (bytes_left + out->bytes_copied > out->size)
    {
        copy_bytes = out->size - out->bytes_copied;
        if (copy_bytes)
        {
            bcopy(src, out->buf + out->bytes_copied, copy_bytes);
            src += copy_bytes;
            bytes_left -= copy_bytes;
            out->bytes_copied += copy_bytes;
        }
        TRACE("pfunc_write: waking output reader\n");
        pthread_mutex_lock(&sp->sump_mtx);
        out->stalled = TRUE;
        pthread_cond_broadcast(&sp->task_output_ready_cond);
        TRACE("pfunc_write: waiting for available output buffer\n");
        while (out->stalled && sp->error_code == 0)
            pthread_cond_wait(&sp->task_output_empty_cond, &sp->sump_mtx);
        pthread_mutex_unlock(&sp->sump_mtx);
        if (sp->error_code != 0)
            return (-1);
    }
    /* copy new record into buffer */
    bcopy(src, out->buf + out->bytes_copied, bytes_left);
    out->bytes_copied += bytes_left;
    return (size);
}


/* pfunc_mutex_lock - lock the auto-allocated mutex for pump functions.
 */
void pfunc_mutex_lock(sp_task_t t)
{
    pthread_mutex_lock(&t->sp->sp_mtx);
}


/* pfunc_mutex_unlock - unlock the auto-allocated mutex for pump functions.
 */
void pfunc_mutex_unlock(sp_task_t t)
{
    pthread_mutex_unlock(&t->sp->sp_mtx);
}


/* done_reading_in_buf - internal routine to mark the task's current
 *                       input buffer as done for reading.  This is
 *                       non-trivial because multiple member threads may
 *                       have to read the same input buffer.
 */
static void done_reading_in_buf(sp_task_t t, int move_to_next_in_buf)
{
    in_buf_t    *ib;
    sp_t        sp = t->sp;

    pthread_mutex_lock(&sp->sump_mtx);

    /* bump up the count of done readers for the input buffer */
    ib = &sp->in_buf[t->curr_in_buf_index % sp->num_in_bufs];
    ib->num_readers_done++;
    /* if all readers are now done, signal the reader thread */
    if (ib->num_readers == ib->num_readers_done)
        pthread_cond_signal(&sp->in_buf_done_cond);

    pthread_mutex_unlock(&sp->sump_mtx);

    if (move_to_next_in_buf)
    {
        /* move this task's current input position to the beginning of
         * the next input buffer.
         */
        t->curr_in_buf_index++;   /* on to next input buffer */
        ib = &sp->in_buf[t->curr_in_buf_index % sp->num_in_bufs];
        t->in_buf = ib->in_buf;
        t->in_buf_bytes = 0;
        t->curr_rec = t->in_buf;
    }
}


/* ready_in_buf - internal routine to insure the current input buffer
 *                for a task is ready for reading.
 */
static void ready_in_buf(sp_task_t t)
{
    in_buf_t     *ib;
    int         len;
    sp_t        sp = t->sp;

    pthread_mutex_lock(&sp->sump_mtx);

    t->first_in_buf = FALSE; /* now that this task has proceeded beyond
                              * its first input buffer, it should stop
                              * at end of the current key group */

    while (sp->error_code == 0 && !sp->input_eof && 
           t->curr_in_buf_index == sp->cnt_in_buf_readable) /*not yet readable*/
    {
        pthread_cond_wait(&sp->in_buf_readable_cond, &sp->sump_mtx);
    }
    
    /* if sp_write_input() has indicated eof and no more readable buffers
     * then that indicates eof for this task.
     */
    if (sp->error_code != 0 ||
        (sp->input_eof &&
         t->curr_in_buf_index == sp->cnt_in_buf_readable))
    {
        t->input_eof = TRUE;
        TRACE("pump%d: eof: error_code: %d, in_buf_bytes is 0 at ib %d\n",
              t->thread_index, sp->error_code, t->curr_in_buf_index);
        t->in_buf = NULL;
        t->in_buf_bytes = 0;
        t->curr_rec = NULL;
    }
    else
    {
        ib = &sp->in_buf[t->curr_in_buf_index % sp->num_in_bufs];
        t->in_buf = ib->in_buf;
        t->in_buf_bytes = ib->in_buf_bytes;        
        t->curr_rec = t->in_buf;
    }

    pthread_mutex_unlock(&sp->sump_mtx);
}


/* is_more_input - internal routine to test if there is more input for
 *                 this pump task.  This routine is called after the
 *                 pump function returns to see if there is additonal
 *                 input data for the task and hence the pump function
 *                 should be called again.
 */
static int is_more_input(sp_task_t t)
{
    if (t->input_eof)   /* if input eof, then false (no more input) */
        return (0);
    
    /* if we are past the first input buffer for this task (because we
     * had to read the remainder of the last task record at the
     * beginning of the second input buffer), then we have hit the end
     * of this task's input.  Note that if REDUCE_BY_KEYS is specified,
     * then the task pump function can read past any record remainder
     * at the beginning of the second input buffer.
     */
    if (!t->first_in_buf && !(t->sp->flags & SP_REDUCE_BY_KEYS))
    {
        done_reading_in_buf(t, t->curr_rec == t->in_buf + t->in_buf_bytes);
        t->input_eof = TRUE;
        return (0);
    }
    
    /* if there are bytes left in the current input buffer, then true
     * (more input).
     */
    TRACE("pump: imi, rec %08x, buf %08x, bytes: %x\n",
          t->curr_rec, t->in_buf, t->in_buf_bytes);
    if (t->curr_rec < t->in_buf + t->in_buf_bytes)
        return (1);

    /* this task is done reading its current input buffer */
    done_reading_in_buf(t, TRUE);
    
    /* if we are not grouping by key difference, then we have hit the
     * end of this task's input.
     */
    if (!(t->sp->flags & SP_REDUCE_BY_KEYS))
    {
        t->input_eof = TRUE;
        return (0);
    }

    /* get next input buffer */
    ready_in_buf(t);
    TRACE("pump: imi, eof %d\n", t->input_eof);
    return (!t->input_eof);
}


/* pfunc_get_rec - get a pointer to the next input record for a pump function.
 */
size_t pfunc_get_rec(sp_task_t t, void *ptr_to_rec_ptr)
{
    void        *buf;
    char        *rec;
    char        *next_rec;
    int         key_offset;
    size_t      len;
    size_t      src_size;
    size_t      trans_size;
    size_t      delim_size = 0;
    in_buf_t    *ib;
    sp_t        sp = t->sp;
    int         new_key_group_beginning = FALSE;

    buf = t->rec_buf;
    
    if (t->input_eof)
        return 0;

    if (REC_TYPE(sp) == SP_UTF_8)
        delim_size = 1;
    
    /* if we have used all the records in the current input buffer...
     * note that we must be on a record boundry at this point.
     */
    if (t->curr_rec >= t->in_buf + t->in_buf_bytes)
    {
        done_reading_in_buf(t, TRUE);  /* done reading input buffer */
        
        /* if we are not grouping records then return no record (0) since
         * we are on a record boundry.
         */
        if (!(sp->flags & SP_REDUCE_BY_KEYS))
        {
            t->input_eof = TRUE;
            return 0;
        }

        ready_in_buf(t);
        if (t->input_eof)
            return 0;
    }

    /* now we have at least a partial record in the input buffer */
    rec = t->curr_rec;
    /* if there is an initial key offset character */
    if (sp->flags & SP_REDUCE_BY_KEYS)
    {
        key_offset = rec[0] - '0';
        rec++;
        new_key_group_beginning =
            (key_offset < sp->group_by_keys && !t->first_group_rec);
    }

    /* if the key offset indicates this is the beginning of a new key group &&
     * this is not the actual first record for this current key group.
     */
    if (!(sp->flags & SP_REDUCE_BY_KEYS) || new_key_group_beginning)
    {
        /* if we are beyond the first input buffer for this task, this
         * rec is not only the beginning of the next key group, it also
         * means we have reached the end of the input for this member
         * task.
         */
        if (!t->first_in_buf)
        {
            done_reading_in_buf(t, FALSE);
            TRACE("pump%d: eof: key_offset: %d at ib %d, curr_rec %08x\n",
                  t->thread_index,
                  (sp->flags & SP_REDUCE_BY_KEYS) ? key_offset : 0,
                  t->curr_in_buf_index, t->curr_rec);
            t->input_eof = TRUE;   /* no need to be inside the mutex because
                                     * only the current thread reads this */
            return 0;
        }
        /*
         * else we are still reading from the first input buffer for this
         * task, but there is at least one more record key group to be
         * processed by this task.
         */
        else if (new_key_group_beginning)
            return 0;
        /* else we are not doing key grouping, go on to return next rec */
    }
    t->first_group_rec = FALSE;  /* only relevant for key grouping */

    /* now copy the record, which may span multiple in_bufs, into the
     * caller's buffer.
     */
    len = 0;
    for (;;)
    {
        src_size = t->in_buf_bytes - (rec - t->in_buf);
        switch (REC_TYPE(sp))
        {
          case SP_UTF_8:
            next_rec = memchr(rec, *(char *)sp->delimiter, src_size);
            if (next_rec != NULL)
            {
                next_rec++;         /* skip over newline */
                trans_size = next_rec - rec;
            }
            else
                trans_size = src_size;
            break;

          case SP_FIXED:
            trans_size = src_size;
            if (trans_size >= sp->rec_size - len)
            {
                trans_size = sp->rec_size - len;
                next_rec = rec + trans_size;
            }
            else
                next_rec = NULL;
            break;
        }            
        if (trans_size + len + delim_size > t->rec_buf_size)
        {
            size_t new_size;

            if (REC_TYPE(sp) == SP_FIXED)
                new_size = sp->rec_size;
            else
                new_size = trans_size + len + delim_size + 50;
            
            if (t->rec_buf == NULL)
                t->rec_buf = (char *)calloc(1, new_size);
            else
                t->rec_buf = realloc(t->rec_buf, new_size);
            if (t->rec_buf == NULL)
            {
                die("pfunc_get_rec: rec_buf increase failed: old %d new %d\n",
                    t->rec_buf_size, new_size);
            }
            t->rec_buf_size = new_size;
            buf = t->rec_buf;
        }
        bcopy(rec, (char *)buf + len, trans_size);
        len += trans_size;
        if (next_rec != NULL)  /* if we found the newline delimiter */
        {
            if (REC_TYPE(sp) == SP_UTF_8)
                ((char *)buf)[len] = '\0';
            break;
        }
        else  /* else we need to get remainder of record from next buffer */
        {
            done_reading_in_buf(t, TRUE);
            ready_in_buf(t);
            if (t->input_eof)
            {
                if (REC_TYPE(sp) == SP_FIXED)
                {
                    die("pfunc_get_rec: partial record of "
                        "%d bytes found at end of input\n", len);
                }
                return 0;
            }
            rec = t->curr_rec;
        }
    }
    t->curr_rec = next_rec;
    *(void **)ptr_to_rec_ptr = buf;
    return len;
}


/* pfunc_get_in_buf - get a pointer to the input buffer for a pump function.
 */
int pfunc_get_in_buf(sp_task_t t, void **buf, size_t *size)
{
    *(char **)buf = t->curr_rec;
    *size = (t->in_buf + t->in_buf_bytes) - t->curr_rec;
    t->curr_rec = t->in_buf + t->in_buf_bytes;
    return (0);
}


/* pfunc_get_out_buf - get a pointer to an output buffer and its size for
 *                     a pump function.
 */
int pfunc_get_out_buf(sp_task_t t, unsigned out_index, void **buf, size_t *size)
{
    sp_t                sp = t->sp;
    struct task_out     *out = t->out + out_index;

    if (out_index >= sp->num_outputs)
    {
        pfunc_error(t, "pfunc_get_out_buf: out_index %d >= num_outputs %d\n",
                    out_index, sp->num_outputs);
    }

    if (out->bytes_copied == out->size)
    {
        sp_t    sp = t->sp;
        
        TRACE("pfunc_get_out_buf: waking output reader\n");
        pthread_mutex_lock(&sp->sump_mtx);
        out->stalled = TRUE;
        pthread_cond_broadcast(&sp->task_output_ready_cond);
        TRACE("pfunc_get_out_buf: waiting for available output buffer\n");
        while (out->stalled && sp->error_code == 0)
            pthread_cond_wait(&sp->task_output_empty_cond, &sp->sump_mtx);
        pthread_mutex_unlock(&sp->sump_mtx);
        if (sp->error_code != 0)
            return (-1);
    }
    *(char **)buf = out->buf + out->bytes_copied;
    *size = out->size - out->bytes_copied;
    return (0);
}


/* pfunc_put_out_buf_bytes - flush bytes that have been placed in a pump
 *                           function's output buffer.  This routine can only
 *                           be used by first calling sp_get_out_buf().
 */
int pfunc_put_out_buf_bytes(sp_task_t t, unsigned out_index, size_t size)
{
    sp_t                sp = t->sp;
    struct task_out     *out = t->out + out_index;

    if (out_index >= sp->num_outputs)
    {
        pfunc_error(t, "pfunc_put_out_buf_bytes: "
                    "out_index %d >= num_outputs %d\n",
                    out_index, sp->num_outputs);
    }
    else if (out->bytes_copied + size > out->size)
    {
        pfunc_error(t, "sp_put_out_buf_bytes: "
                    "aggregate size (%d+%d) is larger than buf size %d\n",
                    out->bytes_copied, size, out->size);
    }
    else
        out->bytes_copied += size;
    return (sp->error_code);
}


/* pfunc_printf - print a formatted string to a pump functions's output.
 */
int pfunc_printf(sp_task_t t, unsigned out_index, const char *fmt, ...)
{
    va_list     ap;
    int         ret;

    va_start(ap, fmt);
    ret = vsnprintf(t->temp_buf, t->temp_buf_size, fmt, ap);
    if (ret >= t->temp_buf_size)
    {
        /* temp buf wasn't big enough.  enlarge it and redo */
        if (t->temp_buf_size != 0)
            free(t->temp_buf);
        va_start(ap, fmt);
        t->temp_buf_size = ret + 10;
        t->temp_buf = (char *)malloc(t->temp_buf_size);
        ret = vsnprintf(t->temp_buf, t->temp_buf_size, fmt, ap);
    }
    return (pfunc_write(t, out_index, t->temp_buf, ret));
}


/* pfunc_error - raise an error for a pump function and define an error string.
 */
int pfunc_error(sp_task_t t, const char *fmt, ...)
{
    va_list     ap;
    int         ret;
    sp_t        sp = t->sp;

    if (sp->error_code != 0)            /* if prior error */
        return SP_PUMP_FUNCTION_ERROR;   /* ignore this one */
    
    va_start(ap, fmt);
    ret = vsnprintf(t->error_buf, t->error_buf_size, fmt, ap);
    if (ret >= t->error_buf_size)
    {
        if (t->error_buf_size != 0)
            free(t->error_buf);
        t->error_buf_size = ret + 1;
        t->error_buf = (char *)malloc(t->error_buf_size);
        va_start(ap, fmt);
        vsnprintf(t->error_buf, t->error_buf_size, fmt, ap);
    }
    pthread_mutex_lock(&sp->sump_mtx);
    pthread_cond_broadcast(&sp->in_buf_readable_cond); /* multiple sp threads*/
    pthread_cond_signal(&sp->in_buf_done_cond);   /* sp_write_input() caller */
    pthread_cond_broadcast(&sp->task_avail_cond);   /* multiple sp threads */
    pthread_cond_signal(&sp->task_drained_cond);  /* sp_write_input() caller */
    pthread_cond_broadcast(&sp->task_output_ready_cond); /* mult sp threads */
    pthread_cond_broadcast(&sp->task_output_empty_cond); /* mult sp threads */
    pthread_mutex_unlock(&sp->sump_mtx);
    
    return SP_PUMP_FUNCTION_ERROR;
}


/* pump_thread_main - the internal "main" routine of a sump pump member thread.
 */
static void *pump_thread_main(void *arg)
{
    int                 size;
    sp_task_t           t;
    int                 thread_index;
    sp_t                sp = (sp_t)arg;
    int                 ret;
    int                 j;

    for (thread_index = 0; thread_index < sp->num_threads; thread_index++)
        if (sp->thread[thread_index] == pthread_self())
            break;
    TRACE("pump thread %d starting\n", thread_index);
    for (;;)
    {
        TRACE("pump%d: waiting for an available task\n", thread_index);
        pthread_mutex_lock(&sp->sump_mtx);
        while (sp->cnt_task_begun == sp->cnt_task_init &&
               sp->error_code == 0 &&
               sp->input_eof == FALSE)
        {
            pthread_cond_wait(&sp->task_avail_cond, &sp->sump_mtx);
        }
        if (sp->error_code != 0 ||
            (sp->input_eof == TRUE && sp->cnt_task_begun == sp->cnt_task_init))
        {
            pthread_mutex_unlock(&sp->sump_mtx);
            TRACE("pump%d: breaking out of for loop: error_code: %d, input_eof %d\n",
                  thread_index, sp->error_code, sp->input_eof);
            break;
        }
        t = &sp->task[sp->cnt_task_begun % sp->num_tasks];
        size = t->in_buf_bytes;
        sp->cnt_task_begun++;
        t->thread_index = thread_index;
        pthread_mutex_unlock(&sp->sump_mtx);

        TRACE("pump%d: calling member func with  %d input bytes\n",
              thread_index, size);
#if 0
        if (ExtraProcess)
        {
            /* Notifiy member process that it has a task to perform */
            sem_post(...);

            for (;;)
            {
                /* Wait for member process to complete or stall */
                sem_wait(...);

                if (complete)
                    break;

                /* notify writer */
                ;

                /* wait for writer */
                ;

                /* notify member process that it can proceed */
                sem_post(...);
            }
        }
        else
#endif
        {
            if (REC_TYPE(sp) == SP_WHOLE_BUF)
            {
                TRACE("pump%d: calling member func() block\n", thread_index);
                ret = (*sp->pump_func)(t, sp->pump_arg);
                TRACE("pump%d: member func returned %d\n", thread_index, ret);
                if (ret)
                {
                    if (t->error_code == 0)
                        t->error_code = ret;
                }
                else
                {
                    done_reading_in_buf(t, TRUE);
                    t->input_eof = TRUE;
                }
            }
            else
            {
                while (is_more_input(t) && t->error_code == 0)
                {
                    TRACE("pump%d: calling member func()\n", thread_index);
                    /* indicate first record in key group not yet read */
                    t->first_group_rec = TRUE;
                    ret = (*sp->pump_func)(t, sp->pump_arg);
                    TRACE("pump%d: member func returned %d, input_eof: %d\n",
                          thread_index, ret, t->input_eof);
                    if (ret && t->error_code == 0)
                        t->error_code = ret;
                }
            }
            TRACE("pump%d: pump_func returns with %d out[0] bytes\n",
                  thread_index, t->out[0].bytes_copied);
            if (t->error_code && sp->error_code == 0)
            {
                sp->error_code = t->error_code;
                sp->error_buf = t->error_buf;
            }
        }

        TRACE("pump%d: waking output reader\n", thread_index);
        TRACE("pump%d: waking input writer\n", thread_index);
        pthread_mutex_lock(&sp->sump_mtx);
        t->output_eof = TRUE;
        pthread_cond_broadcast(&sp->task_output_ready_cond);
        pthread_mutex_unlock(&sp->sump_mtx);
        /* NOTA BENE: do not use "t" pointer after this point since the
         * struct that it points to can be reused immediately */
    }
    TRACE("pump%d: exiting\n", thread_index);

    return (NULL);
}


/* sp_read_output - read bytes from the specified output of a sump pump.
 */
ssize_t sp_read_output(sp_t sp, unsigned index, void *buf, ssize_t size)
{
    int                 out_eof;
    ssize_t             bytes_returned = 0;
    ssize_t             src_remaining = size;
    ssize_t             dst_remaining;
    ssize_t             trans_size;
    char                *trans_src;
    char                *trans_dst;
    sp_task_t           t;
    struct task_out     *out;

    TRACE("sp_read_output[%d]: buf %08x, size %d\n", index, buf, size);

    if (sp->error_code)
        return (-1);

    if (sp->flags & SP_SORT)
    {
        int     ret;

        if (sp->sort_state == SORT_INPUT)
        {
            pthread_mutex_lock(&sp->sump_mtx);
            while (sp->error_code == 0 && sp->sort_state == SORT_INPUT)
                pthread_cond_wait(&sp->task_output_ready_cond, &sp->sump_mtx);
            pthread_mutex_unlock(&sp->sump_mtx);
            if (sp->error_code != 0)
                return (-1);
        }
        if (sp->sort_state == SORT_DONE)
            return (0);

        while (bytes_returned < size)
        {
            /* if there is no data in the sort_temp_buf, then fill it.
             */
            if (sp->out[0].partial_bytes_copied == 0)
            {
                /* call nsort_return_recs() until it does NOT return a buffer
                 * too small error.
                 */
                for (;;)
                {
                    char    *temp;
                
                    sp->sort_temp_buf_bytes = sp->sort_temp_buf_size;
                    ret = (*Nsort_return_recs)(sp->sort_temp_buf,
                                               &sp->sort_temp_buf_bytes,
                                               &sp->nsort_ctx);
                    if (ret != NSORT_RETURN_BUF_SMALL)
                        break;
                    temp = sp->sort_temp_buf;
                    sp->sort_temp_buf_size *= 2;
                    sp->sort_temp_buf = (char *)malloc(sp->sort_temp_buf_size);
                    if (sp->sort_temp_buf == NULL)
                    {
                        sp->error_code = SP_MEM_ALLOC_ERROR;
                        return (-1);
                    }
                    free(temp);
                }
                if (ret == NSORT_END_OF_OUTPUT)
                {
                    pthread_mutex_lock(&sp->sump_mtx);
                    sp->sort_state = SORT_DONE;
                    pthread_cond_broadcast(&sp->task_output_ready_cond);
                    pthread_mutex_unlock(&sp->sump_mtx);
                    return (bytes_returned);
                }
                else if (ret < 0)
                {
                    post_nsort_error(sp, ret);
                    return (-1);
                }
            }

            src_remaining =
                sp->sort_temp_buf_bytes - sp->out[0].partial_bytes_copied;
            dst_remaining = size - bytes_returned;
            trans_size = dst_remaining;
            trans_src = sp->sort_temp_buf + sp->out[0].partial_bytes_copied;
            trans_dst = (char *)buf + bytes_returned;
            if (dst_remaining <= src_remaining)
            {
                /* we will fill the caller's buffer completely.
                 */
                bcopy(trans_src, trans_dst, dst_remaining);
                bytes_returned += dst_remaining;
                sp->out[0].partial_bytes_copied += dst_remaining;
                break;
            }

            /* the sort temp buffer will be completely copied out,
             * and we still need more.
             */
            bcopy(trans_src, trans_dst, src_remaining);
            bytes_returned += src_remaining;
            /* indicate sort temp output buffer needes to be refilled */
            sp->out[0].partial_bytes_copied = 0;
        }
        return (bytes_returned);
    }

    if (index >= sp->num_outputs)
        return (-1);
            
    for (;;)
    {
        TRACE("sp_read_output: out[%d].cnt_task_drained: %d, "
              "partial_bytes_copied: %d\n",
              index, sp->out[index].cnt_task_drained,
              sp->out[index].partial_bytes_copied);
        t = &sp->task[sp->out[index].cnt_task_drained % sp->num_tasks];
        out = t->out + index;

        /* if we aren't in the middle of copy out a task's buffer, then
         * we must potentially wait for the output of the next task.
         */
        if (sp->out[index].partial_bytes_copied == 0)
        {
            TRACE("sp_read_output: waiting for input\n");
            pthread_mutex_lock(&sp->sump_mtx);

            /* while 1) a sump pump error hasn't occurred.
             * and   2) it's not the case that
             *          a) EOF on input has been reached
             *          b) all initialized tasks have begun (been taken), and
             *          c) all taken tasks have had their output read, and
             * and   3) it's not the case oldest member task is either
             *          done or stalled
             */
            while (sp->error_code == 0 &&
                   !(out_eof = (sp->input_eof &&
                                sp->cnt_task_init == sp->cnt_task_begun &&
                                sp->cnt_task_begun == sp->out[index].cnt_task_drained)) &&
                   !(sp->out[index].cnt_task_drained < sp->cnt_task_begun &&
                     (out->stalled || t->output_eof)))
            {
                TRACE("sp_read_output: waiting\n");
                TRACE("sp_read_output: cnt_task_init: %d\n",
                      sp->cnt_task_init);
                TRACE("sp_read_output: cnt_task_begun: %d\n",
                      sp->cnt_task_begun);
                TRACE("sp_read_output: out[%d].cnt_task_drained: %d\n",
                      index, sp->out[index].cnt_task_drained);
                TRACE("sp_read_output: out->bytes_copied: %d\n",
                      out->bytes_copied);
                TRACE("sp_read_output: t->in_buf_bytes: %d\n",
                      t->in_buf_bytes);
                TRACE("sp_read_output: out->stalled: %d\n", out->stalled);
                TRACE("sp_read_output: t->output_eof: %d\n", t->output_eof);
                pthread_cond_wait(&sp->task_output_ready_cond, &sp->sump_mtx);
            }
            TRACE("sp_read_output: DONE WAITING\n");
            TRACE("sp_read_output: error_code: %d\n", sp->error_code);
            TRACE("sp_read_output: cnt_task_init: %d\n", sp->cnt_task_init);
            TRACE("sp_read_output: cnt_task_begun: %d\n", sp->cnt_task_begun);
            TRACE("sp_read_output: out[%d].cnt_task_drained: %d\n",
                  index, sp->out[index].cnt_task_drained);
            TRACE("sp_read_output: out->bytes_copied: %d\n",
                  out->bytes_copied);
            TRACE("sp_read_output: t->in_buf_bytes: %d\n", t->in_buf_bytes);
            TRACE("sp_read_output: out->stalled: %d\n", out->stalled);
            TRACE("sp_read_output: t->output_eof: %d\n", t->output_eof);
            TRACE("sp_read_output: out_eof: %d\n", out_eof);
            pthread_mutex_unlock(&sp->sump_mtx);
            if (sp->error_code || out_eof)
                break;
        }  
        src_remaining =
            out->bytes_copied - sp->out[index].partial_bytes_copied;
        dst_remaining = size - bytes_returned;
        trans_size = dst_remaining;
        trans_src = t->out[index].buf + sp->out[index].partial_bytes_copied;
        trans_dst = (char *)buf + bytes_returned;
        if (dst_remaining < src_remaining)
        {
            /* we will fill the output buffer and still leave some bytes
             * in the sump pump task's output buffer.
             */
            bcopy(trans_src, trans_dst, dst_remaining);
            bytes_returned += dst_remaining;
            sp->out[index].partial_bytes_copied += dst_remaining;
            break;
        }

        /* the sump pump task's output buffer will be completely copied out.
         */
        bcopy(trans_src, trans_dst, src_remaining);
        bytes_returned += src_remaining;
        /* indicate new sump pump task output is needed */
        sp->out[index].partial_bytes_copied = 0;  
        
        TRACE("sp_read_output: waking reader thread\n");
        pthread_mutex_lock(&sp->sump_mtx);
        if (t->out[index].stalled)
        {
            /* we have copied the bytes in the buf.  clear the buf
             * and stall indicator, then wake stalled thread.  since
             * there may be more than one thread waiting on the
             * task_output_empty_cond, use broadcast.
             */
            t->out[index].bytes_copied = 0;
            t->out[index].stalled = FALSE;
            pthread_cond_broadcast(&sp->task_output_empty_cond);
        }
        else
        {
            /* increment per-output task output drained count */
            sp->out[index].cnt_task_drained++;
            TRACE("sp_read_output: sp->out[%d].cnt_task_drained incr to %d\n",
                  index, sp->out[index].cnt_task_drained);
            TRACE("sp_read_output: t->outs_drained before incr is: %d\n",
                  t->outs_drained);

            /* if all output buffers for this task have been drained */
            if (++t->outs_drained == sp->num_outputs)  
            {
                /* increment sump pump task drained count */
                sp->cnt_task_drained++;
                TRACE("sp_read_output: sp->cnt_task_drained incr to: %d\n",
                      sp->cnt_task_drained);
                pthread_cond_signal(&sp->task_drained_cond);
            }
        }

        pthread_mutex_unlock(&sp->sump_mtx);

        if (bytes_returned == size)
            break;
    }

    if (sp->error_code)
        bytes_returned = -1;
    TRACE("sp_read_output: returning %d bytes\n", bytes_returned);
    return (bytes_returned);
}


/* link_main - internal "main" routine for a thread that links an output
 *             of a sump pump to the input of another sump pump.
 */
static void *link_main(void *arg)
{
    sp_link_t   sp_link = (sp_link_t)arg;
    ssize_t     size;

    while ((size = sp_read_output(sp_link->out_sp,
                                  sp_link->out_index,
                                  sp_link->buf,
                                  sp_link->buf_size)) > 0)
    {
        if (sp_write_input(sp_link->in_sp, sp_link->buf, size) != size)
        {
            TRACE("link_main: sp_write_input() returned wrong size\n");
            sp_link->error_code = SP_WRITE_ERROR;
            return (NULL);
        }
    }
    sp_write_input(sp_link->in_sp, NULL, size);/* need to handle err case?*/
    if (size < 0)
        sp_link->error_code = SP_UPSTREAM_ERROR;
    return (NULL);
}


/* sp_link - start a link/connection between an output
 *           of a sump pump to the input of another sump pump.
 */
int sp_link(sp_t out_sp, unsigned out_index, sp_t in_sp)
{
    struct sp_link      *sp_link;
    int                 ret;
    int                 key_diff_output;
    int                 reduce_by_input;

    sp_link = (struct sp_link *)calloc(1, sizeof(struct sp_link));
    if (sp_link == NULL)
        return (SP_MEM_ALLOC_ERROR);
    key_diff_output = (out_sp->flags & SP_KEY_DIFF) ? 1 : 0;
    reduce_by_input = (in_sp->flags & SP_REDUCE_BY_KEYS) ? 1 : 0;
    if (key_diff_output ^ reduce_by_input)
        return (SP_REDUCE_BY_MISMATCH);
    sp_link->out_sp = out_sp;
    sp_link->out_index = out_index;
    sp_link->in_sp = in_sp;
    sp_link->buf_size = 4096;
    sp_link->buf = (char *)malloc(sp_link->buf_size);
    if (sp_link->buf == NULL)
        return (SP_MEM_ALLOC_ERROR);
    if ((ret = pthread_create(&sp_link->thread, NULL, link_main, sp_link)))
        die("sp_start_link: pthread_create() ret: %d\n", ret);

    return (SP_OK);
}


#define DIFF_DRCTV      " -diff"
#define STAT_DRCTV      " -stat"


/* sp_start_sort - start an nsort instance with a sump pump wrapper so
 *                 that its input or output can be assigned to a file or
 *                 linked to another sump pump.  For instance, the sort
 *                 input or output can be linked to sump pump performing
 *                 record pumping such as "map" on input and "reduce"
 *                 for output.
 * Parameters:
 *      sp -       Pointer to where to return newly allocated sp_t 
 *                 identifier that will be used in as the first
 *                 argument to all subsequent sp_*() calls.
 *      flags -    Unsigned int with the following possible values:
 *                   SP_KEY_DIFF     Output records should be preceded
 *                                   by a single byte that indicates how
 *                                   many keys in this record are the
 *                                   same as in the previous record.
 *      def -      Nsort sort definition string. 
 */
int sp_start_sort(sp_t *caller_sp,
                  unsigned flags,
                  char *def_fmt,
                  ...)
{
    int         ret;
    char        *def_plus = NULL;
    sp_t        sp;
    size_t      def_len;
    char        *def;
    char        thread_drctv[30];

    *caller_sp = NULL;  /* assume the worst for now */
    sp = (sp_t)calloc(1, sizeof(struct sump));
    if (sp == NULL)
        return (SP_MEM_ALLOC_ERROR);
    sp->error_buf_size = ERROR_BUF_SIZE;
    sp->error_buf = (char *)calloc(1, sp->error_buf_size);
    if (sp->error_buf == NULL)
        return (SP_MEM_ALLOC_ERROR);
    *caller_sp = sp;  /* allow access to error_buf even if failure */
    /* fill in default parameters */
    sp->num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    sprintf(thread_drctv, "-process=%d ", sp->num_threads);
    if (sp->num_outputs > 32)
        sp->num_outputs = 32;
    sp->num_outputs = 1;
    sp->out = (struct sump_out *)calloc(1, sizeof(struct sump_out));
    sp->delimiter = (void *)"\n";
    sp->rec_size = 0;

    if (link_in_nsort() != 0)    /* if error */
    {
        sp->error_code = SP_NSORT_LINK_FAILURE;
        sp->sort_state = SORT_DONE;
        return (sp->error_code);
    }
    
    sp->flags = flags;
    sp->flags |= SP_SORT;

    if (def_fmt != NULL)
    {
        va_list ap;
        size_t  def_len;
        
        va_start(ap, def_fmt);
        def_len = vsnprintf(NULL, 0, def_fmt, ap);
        def = (char *)calloc(def_len + 1, 1);
        va_start(ap, def_fmt);
        if (vsnprintf(def, def_len + 1, def_fmt, ap) != def_len)
            die("sp_start_sort: vnsprintf failed to return %d\n", def_len);
    }
    else
        def = "";

    def_len = strlen(def) + 1;  /* plus '\0' */
    def_len += strlen(STAT_DRCTV);
    if (flags & SP_KEY_DIFF)  /* if appending " -diff" directive */
        def_len += strlen(DIFF_DRCTV);
    def_len += strlen(thread_drctv);
    def_plus = (char *)malloc(def_len);
    if (def_plus == NULL)
        return (SP_MEM_ALLOC_ERROR);
    strcpy(def_plus, thread_drctv); /* goes first so caller can override */
    strcat(def_plus, def);
    strcat(def_plus, STAT_DRCTV);
    if (flags & SP_KEY_DIFF)
        strcat(def_plus, DIFF_DRCTV);
    ret = (*Nsort_define)(def_plus, 0, NULL, &sp->nsort_ctx);
    free(def_plus);
    if (def_fmt != NULL)
        free(def);
    if (ret < 0)
    {
        char *msg = (*Nsort_message)(&sp->nsort_ctx);
        
        sp->sort_error = ret;
        sp->error_code = SP_SORT_DEF_ERROR;
        if (msg == NULL)
            msg = "No Nsort error message";
        strncpy(sp->error_buf, msg, sp->error_buf_size);
        sp->error_buf[sp->error_buf_size - 1] = '\0';  /* handle overflow */
        sp->sort_state = SORT_DONE;
        return (SP_SORT_DEF_ERROR);
    }
    sp->sort_state = SORT_INPUT;
    pthread_mutex_init(&sp->sump_mtx, NULL);
    /* use output ready cond for state changes */
    pthread_cond_init(&sp->task_output_ready_cond, NULL);
    sp->sort_temp_buf_size = sp->out[0].buf_size ? sp->out[0].buf_size : 4096;
    if ((sp->sort_temp_buf = (char *)malloc(sp->sort_temp_buf_size)) == NULL)
        return (SP_MEM_ALLOC_ERROR);
    return (SP_OK);
}


/* sp_get_sort_stats - get a string containing the nsort statistics report
 *                     for an nsort sump pump that has completed.
 */
const char *sp_get_sort_stats(sp_t sp)
{
    const char  *ret;
    
    if (sp->error_code)
        return ("no stats because of nsort error");

    if (!(sp->flags & SP_SORT))
        return (NULL);
    if ((ret = (*Nsort_get_stats)(&sp->nsort_ctx)) == NULL)
        ret = "Nsort_get_stats() failure\n";
    return (ret);
}


/* get_output_index - internal routine to read an output index preceded by
 *                    a "[" and followed by "]=".
 */
static int get_output_index(sp_t sp, char **caller_p)
{
    int         index;
    char        *p = *caller_p;
    
    if (*p++ != '[')
    {
        syntax_error(sp, p, "expected '['");
        return 0;
    }
    index = (int)get_numeric_arg(sp, &p);
    if (index < 0 || index >= sp->num_outputs)
    {
        syntax_error(sp, p,
                     "output index is greater than the number of outputs");
        return 0;
    }
    if (*p++ != ']')
    {
        syntax_error(sp, p, "expected ']'");
        return 0;
    }
    if (*p++ != '=')
    {
        syntax_error(sp, p, "expected '='");
        return 0;
    }
    *caller_p = p;
    return (index);
}


/* get_string_arg - internal routine to scan and return a string
 */
const char *get_string_arg(char **caller_p)
{
    char        *begin_p = *caller_p;
    char        *p;
    char        *ret;
    int         i;

    /* scan string up to the next white space character */
    p = begin_p;
    while (!isspace(*p) && *p != '\0')
        p++;
    *caller_p = p;

    /* allocate space for the return string and copy contents to it */
    ret = (char *)calloc(1, p - begin_p + 1);
    for (i = 0; i < p - begin_p; i++)
        ret[i] = begin_p[i];
    ret[p - begin_p] = '\0';
    return (ret);
}


/* sp_argv_to_str - bundle up the specified argv and return it as a string.
 *                  For instance if argc is 2, argv[0] is "TASKS=2" and
 *                  argv[1] is "THEADS=3", then return the string
 *                  "TASKS=2 THREADS=3".
 */
const char *sp_argv_to_str(char *argv[], int argc)
{
    int                 i;
    char                *str = NULL;
    int                 n_chars;

    n_chars = 1;      /* '\0' */
    for (i = 0; i < argc; i++)
        n_chars += 1 + strlen(argv[i]);   /* ' ' + argv[i] */
    str = (char *)calloc(sizeof(char), n_chars + 1);
    strcpy(str, argc == 0 ? "" : argv[0]);
    for (i = 1; i < argc; i++)
    {
        strcat(str, " ");
        strcat(str, argv[i]);
    }
    return (str);
}


/* sp_start - Start a sump pump
 *
 * Parameters:
 *      sp -          Pointer to where to return newly allocated sp_t 
 *                    identifier that will be used in as the first argument
 *                    to all subsequent sp_*() calls.
 *      pump_func - Pointer to pump function that will be called by
 *                    multiple sump pump threads at once.
 *      flags -       Unsigned int with the following possible values:
 *                    SP_UTF_8        Records consist of utf-8 characters with
 *                                    newline as the record delimiter
 *                    SP_ASCII        Same as SP_UTF_8
 *                    SP_FIXED        Input records are a fixed number of bytes
 *                    SP_WHOLE_BUF    There are no input records, instead 
 *                                    there is only an input buffer
 *      arg_fmt -     Printf-format-like string that can be used with
 *                    subsequent arguments as follows:
 *                    REDUCE_BY_KEYS=%d   Group input records by the given
 *                                        number of keys for the purpose of
 *                                        reducing them.  The sump pump input
 *                                        should be coming from an nsort
 *                                        instance where the SP_KEY_DIFF
 *                                        flag has been set.
 *                    IN_FILE=%s          Input file name for the sump pump
 *                                        input.  if not specified, the input
 *                                        should be written into the sump pump
 *                                        either by calls to sp_write_input()
 *                                        or sp_start_link()
 *                    IN_BUF_SIZE=%d      Overrides default input buffer size
 *                    IN_BUFS=%d          Overrides default number of input
 *                                        buffers
 *                    OUTPUTS=%d          Overrides default number of output
 *                                        streams (1)
 *                    TASKS=%d            Overrides default number of output
 *                                        tasks
 *                    THREADS=%d          Overrides default number of threads
 *                                        that are used to execute the pump
 *                                        function in parallel
 *                    OUT_BUF_SIZE[%d]=%d The sizes of each tasks output buffer
 *                                        for the specified output
 *                    OUT_FILE[%d]=%s     The output file name for the
 *                                        specified output.  if not defined,
 *                                        the output should be read either
 *                                        by calls to sp_read_output() or by
 *                                        sp_start_link().
 *                    REC_SIZE=%d         Defines the record size in bytes
 *                                        when the SP_FIXED flag is used
 *      ...           potential subsequent arguments to prior arg_fmt
 */
int sp_start(sp_t *caller_sp,
             sp_pump_t pump_func,
             unsigned flags,
             char *arg_fmt,
             ...)
{
    sp_t                sp;
    char                *s;
    int                 i;
    int                 j;
    int                 ret;
    char                *p;
    char                *kw;
    int                 index;
    
    if (TraceFp == NULL &&
        (s = getenv("SUMP_PUMP_TRACE")) != NULL &&
        strlen(s) > 0)
    {
        if (!strcmp(s, "<stdout>"))
            TraceFp = stdout;
        else if (!strcmp(s, "<stderr>"))
            TraceFp = stderr;
        else
        {
            TraceFp = fopen(s, "a+");
            if (TraceFp == NULL)
                fprintf(stderr, "can't open SUMP_PUMP_TRACE=%s\n", s);
            else
            {
                time_t  now = time(NULL);
                trace("begin new sump pump trace, %s", ctime(&now));
            }
        }
    }
    
    *caller_sp = NULL;
    sp = (sp_t)calloc(1, sizeof(struct sump));
    if (sp == NULL)
        return (SP_MEM_ALLOC_ERROR);
    sp->error_buf_size = ERROR_BUF_SIZE;
    sp->error_buf = (char *)calloc(1, sp->error_buf_size);
    if (sp->error_buf == NULL)
        return (SP_MEM_ALLOC_ERROR);
    
    /* fill in default parameters */
    sp->pump_arg = NULL;
    sp->num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    sp->num_in_bufs = 3 * sp->num_threads;
    sp->num_tasks = 3 * sp->num_threads;
    sp->in_buf_size = (1 << 18);
    sp->num_outputs = 1;
    sp->out = (struct sump_out *)calloc(1, sizeof(struct sump_out));
    sp->out[0].buf_size = (1 << 18);
    sp->delimiter = (void *)"\n";
    sp->rec_size = 0;

    sp->pump_func = pump_func;
    sp->flags = flags;
    if (REC_TYPE(sp) == 0)
    {
        start_error(sp, "sp_start: a record type must be specified\n");
        return (sp->error_code);
    }
    if (REC_TYPE(sp) == SP_UTF_8)
    {
        if (strlen((char *)sp->delimiter) > 1)
        {
            start_error(sp, "sp_start: "
                        "currently only single-char delimiters are allowed\n");
            return (sp->error_code);
        }
    }
    else if (REC_TYPE(sp) == SP_UNICODE)
    {
        start_error(sp,
                    "sp_start: unicode records are currently not supported\n");
        return (sp->error_code);
    }
    else if (REC_TYPE(sp) == SP_FIXED)
    {
        /* nothing for now */
    }
    else if (REC_TYPE(sp) == SP_WHOLE_BUF)
    {
        /* nothing for now */
    }
    else
    {
        start_error(sp, "sp_start: multiple record types specified\n");
        return (sp->error_code);
    }

    if (arg_fmt != NULL)
    {
        va_list ap;
        char    *args;
        size_t  args_size;
        
        va_start(ap, arg_fmt);
        args_size = vsnprintf(NULL, 0, arg_fmt, ap);
        args = (char *)calloc(args_size + 1, 1);
        va_start(ap, arg_fmt);
        if (vsnprintf(args, args_size + 1, arg_fmt, ap) != args_size)
        {
            start_error(sp, "sp_start: "
                        "vnsprintf failed to return %d\n", args_size);
            return (sp->error_code);
        }
        p = args;
        for (;;)
        {
            while (isspace(*p))    /* ignore leading white space chars */
                p++;
            if (*p == '-')         /* skip over optional '-' char */
                p++;
            if (*p == '\0')
                break;
            else if (kw = "DEFAULT_FILE_MODE=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                if (kw = "BUFFERED", !strncasecmp(p, kw, strlen(kw)))
                {
                    p += strlen(kw);
                    Default_file_mode = MODE_BUFFERED;
                }
                else if (kw = "DIRECT", !strncasecmp(p, kw, strlen(kw)))
                {
                    p += strlen(kw);
                    Default_file_mode = MODE_DIRECT;
                }
                else
                    syntax_error(sp, p, "unrecognized file access mode");
            }
            else if (kw = "IN_BUF_SIZE=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                sp->in_buf_size = get_numeric_arg(sp, &p);
                sp->in_buf_size *= get_scale(&p);
            }
            else if (kw = "IN_FILE=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                /* get input file here */
                sp->in_file = get_string_arg(&p);
            }
            else if (kw = "IN_BUFS=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                sp->num_in_bufs = get_numeric_arg(sp, &p);
            }
            else if (kw = "OUTPUTS=", !strncasecmp(p, kw, strlen(kw)))
            {
                unsigned        num_outputs;
                
                p += strlen(kw);
                num_outputs = get_numeric_arg(sp, &p);
                if (num_outputs > sp->num_outputs)
                {
                    sp->out = (struct sump_out *)
                        realloc(sp->out, num_outputs * sizeof(struct sump_out));
                    if (sp->out == NULL)
                    {
                        start_error(sp, "set_num_outputs, realloc() failed\n");
                        return (sp->error_code);
                    }
                }
                for (i = sp->num_outputs; i < num_outputs; i++)
                    sp->out[i].buf_size = sp->out[0].buf_size;
                sp->num_outputs = num_outputs;
            }
            else if (kw = "TASKS=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                sp->num_in_bufs = sp->num_tasks = get_numeric_arg(sp, &p);
            }
            else if (kw = "THREADS=", !strncasecmp(p, kw, strlen(kw)))
            {
                int     num_threads;
                
                p += strlen(kw);
                num_threads = get_numeric_arg(sp, &p);
                if (num_threads > 0)
                    sp->num_threads = num_threads;
            }
            else if (kw = "OUT_BUF_SIZE", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                index = get_output_index(sp, &p);
                sp->out[index].buf_size = (int)get_numeric_arg(sp, &p);
                sp->out[index].buf_size *= get_scale(&p);
            }
            else if (kw = "OUT_FILE", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                index = get_output_index(sp, &p);
                sp->out[index].file = get_string_arg(&p);
            }
            else if (kw = "REC_SIZE=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                sp->rec_size = (int)get_numeric_arg(sp, &p);
            }
            else if (kw = "REDUCE_BY_KEYS=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                sp->group_by_keys = get_numeric_arg(sp, &p);
                sp->flags |= SP_REDUCE_BY_KEYS;
            }
            else if (kw = "RW_TEST_SIZE=", !strncasecmp(p, kw, strlen(kw)))
            {
                p += strlen(kw);
                Default_rw_test_size = get_numeric_arg(sp, &p);
                Default_rw_test_size *= get_scale(&p);
            }
            else
                syntax_error(sp, p, "unrecognized keyword");

            if (sp->error_code)
                return (sp->error_code);
        }
        free(args);
    }
    
    if (REC_TYPE(sp) == SP_FIXED)
    {
        if (sp->rec_size <= 0)
        {
            start_error(sp, "sp_start: "
                        "fixed-size records must have a declared size\n");
            return (sp->error_code);
        }
    }

    /* alloc rec output buffers for each task */
    sp->task = (sp_task_t)calloc(sp->num_tasks, sizeof(struct sp_task));
    for (i = 0; i < sp->num_tasks; i++)
    {
        sp->task[i].out = (struct task_out *)
            calloc(sp->num_outputs, sizeof (struct task_out));
        sp->task[i].error_buf_size = ERROR_BUF_SIZE;
        sp->task[i].error_buf = (char *)calloc(1, sp->task[i].error_buf_size);
        if (sp->task[i].error_buf == NULL)
            return (SP_MEM_ALLOC_ERROR);
        for (j = 0; j < sp->num_outputs; j++)
        {
            sp->task[i].out[j].buf = (void *)malloc(sp->out[j].buf_size);
            sp->task[i].out[j].size = sp->out[j].buf_size;
        }
        sp->task[i].sp = sp;
    }
    /* alloc input buffers */
    sp->in_buf = (in_buf_t *)calloc(sp->num_in_bufs, sizeof(in_buf_t));
    for (i = 0; i < sp->num_in_bufs; i++)
    {
        size_t  buf_size;

        /* round up buf size to page size multiple */
        buf_size = ((sp->in_buf_size + Page_size - 1) / Page_size) * Page_size;
        init_zero_fd();
        sp->in_buf[i].in_buf = mmap(NULL, buf_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE, Zero_fd, 0);
        if (sp->in_buf[i].in_buf == MAP_FAILED)
            return (SP_MEM_ALLOC_ERROR);
        sp->in_buf[i].in_buf_size = sp->in_buf_size;
    }

    /* create mutexes and conditions */
    pthread_mutex_init(&sp->sump_mtx, NULL);
    pthread_mutex_init(&sp->sp_mtx, NULL);
    pthread_cond_init(&sp->in_buf_readable_cond, NULL);
    pthread_cond_init(&sp->in_buf_done_cond, NULL);
    pthread_cond_init(&sp->task_avail_cond, NULL);
    pthread_cond_init(&sp->task_drained_cond, NULL);
    pthread_cond_init(&sp->task_output_ready_cond, NULL);
    pthread_cond_init(&sp->task_output_empty_cond, NULL);

    /* create thread sump members */
    sp->thread = (pthread_t *)calloc(sp->num_threads, sizeof(pthread_t *));
    for (i = 0; i < sp->num_threads; i++)
    {
        ret =
            pthread_create(&sp->thread[i], NULL, pump_thread_main, (void *)sp);
        if (ret)
            die("pthread_create() failed: %d\n", ret);
    }

    for (i = 0; i < sp->num_outputs; i++)
    {
        if (sp->out[i].file != NULL)
        {
            /* start an output file connection */
            sp->out[i].file_sp =
                sp_open_file_dst(sp, i, sp->out[i].file,
                                 O_WRONLY | O_CREAT | O_TRUNC);
            if (sp->out[i].file_sp == NULL)
            {
                perror(sp->out[i].file);
                return (SP_FILE_OPEN_ERROR);
            }
        }
    }
    
    if (sp->in_file != NULL)
    {
        /* start an input file connection */
        sp->in_file_sp = sp_open_file_src(sp, sp->in_file, O_RDONLY);
        if (sp->in_file_sp == NULL)
        {
            perror(sp->in_file);
            return (SP_FILE_OPEN_ERROR);
        }
    } 
    *caller_sp = sp;
    return (SP_OK);
}


/* sp_get_error_string - return an error message string
 */
const char *sp_get_error_string(sp_t sp, int error_code)
{
    const char  *err_code_str;

    if (sp != NULL && sp->error_buf[0] != '\0')
        return (sp->error_buf);
    if (error_code == 0 && sp != NULL)
        error_code = sp->error_code;
    switch (error_code)
    {
      case SP_FILE_READ_ERROR:
        err_code_str = "SP_FILE_READ_ERROR: file read error";
        break;

      case SP_FILE_WRITE_ERROR:
        err_code_str = "SP_FILE_WRITE_ERROR: file write error";
        break;

      case SP_UPSTREAM_ERROR:
        err_code_str = "SP_UPSTREAM_ERROR: upstream error";
        break;

      case SP_REDUNDANT_EOF:
        err_code_str = "SP_REDUNDANT_EOF: redundant eof";
        break;

      case SP_MEM_ALLOC_ERROR:
        err_code_str = "SP_MEM_ALLOC_ERROR: memory allocation error";
        break;

      case SP_FILE_OPEN_ERROR:
        err_code_str = "SP_FILE_OPEN_ERROR: file open error";
        break;

      case SP_WRITE_ERROR:
        err_code_str = "SP_WRITE_ERROR: write error";
        break;

      case SP_SORT_DEF_ERROR:
        err_code_str = "SP_SORT_DEF_ERROR: sort definition error";
        break;

      case SP_SORT_EXEC_ERROR:
        err_code_str = "SP_SORT_EXEC_ERROR: sort execution error";
        break;

      case SP_REDUCE_BY_MISMATCH:
        err_code_str = "SP_REDUCE_BY_MISMATCH: reduce-by mismatch";
        break;

      case SP_SORT_INCOMPATIBLE:
        err_code_str = "SP_SORT_INCOMPATIBLE: a sort sump pump is incompatible"
            " with buffer-at-a-time i/o, specify the input or"
            " output file as a sort parameter instead";
        break;

      case SP_BUF_INDEX_ERROR:
        err_code_str = "SP_BUF_INDEX_ERROR: the specified buffer index is"
            " out-of-range";
        break;

      case SP_OUTPUT_INDEX_ERROR:
        err_code_str = "SP_OUTPUT_INDEX_ERROR: the specified output index is"
            " equal to or larger than the number of outputs";
        break;

      case SP_START_ERROR:
        err_code_str = "SP_START_ERROR: an error occured during sp_start()";
        break;

      case SP_NSORT_LINK_FAILURE:
        err_code_str =
            "SP_NSORT_LINK_FAILURE: link attempt to nsort library failed. "
            "Is nsort installed?";
        break;

      case SP_PUMP_FUNCTION_ERROR:
        err_code_str = "Pump function error";
        break;

      default:
        err_code_str = "Unknown Error";
    }
    return (err_code_str);
}


#if 0
/* sp_get_time_us  - return elapsed time in microseconds.
 *
 *      The caller can discard the upper 32 bits *only* when timing intervals
 *      less than ~4000 seconds = 1 hour 11 minutes.
 */
sp_time_t sp_get_time_us(void)
{
    struct timeval      time;

    if (gettimeofday(&time, NULL) < 0)
        bzero(&time, sizeof(struct timeval));
    return (time.tv_sec * (sp_time_t)1000 * 1000) + time.tv_usec;
}
#endif

