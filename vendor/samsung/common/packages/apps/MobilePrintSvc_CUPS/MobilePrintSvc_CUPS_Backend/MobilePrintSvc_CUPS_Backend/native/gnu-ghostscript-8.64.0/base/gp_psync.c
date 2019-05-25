/* Copyright (C) 2001-2006 Artifex Software, Inc.
   All Rights Reserved.
  
  This file is part of GNU ghostscript

  GNU ghostscript is free software; you can redistribute it and/or
  modify it under the terms of the version 2 of the GNU General Public
  License as published by the Free Software Foundation.

  GNU ghostscript is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  ghostscript; see the file COPYING. If not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

/* $Id: gp_psync.c,v 1.9 2009/04/19 13:54:27 Arabidopsis Exp $ */
/* POSIX pthreads threads / semaphore / monitor implementation */
#include "std.h"
#include "malloc_.h"
#include <pthread.h>
#include "gserror.h"
#include "gserrors.h"
#include "gpsync.h"

/*
 * Thanks to Larry Jones <larry.jones@sdrc.com> for this revision of
 * Aladdin's original code into a form that depends only on POSIX APIs.
 */

/*
 * Some old versions of the pthreads library define
 * pthread_attr_setdetachstate as taking a Boolean rather than an enum.
 * Compensate for this here.
 */
#ifndef PTHREAD_CREATE_DETACHED
#  define PTHREAD_CREATE_DETACHED 1
#endif

/* ------- Synchronization primitives -------- */

/* Semaphore supports wait/signal semantics */

typedef struct pt_semaphore_t {
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} pt_semaphore_t;

uint
gp_semaphore_sizeof(void)
{
    return sizeof(pt_semaphore_t);
}

/*
 * This procedure should really check errno and return something
 * more informative....
 */
#define SEM_ERROR_CODE(scode)\
  (scode != 0 ? gs_note_error(gs_error_ioerror) : 0)

int
gp_semaphore_open(gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode;

    if (!sema)
	return -1;		/* semaphores are not movable */
    sem->count = 0;
    scode = pthread_mutex_init(&sem->mutex, NULL);
    if (scode == 0)
	scode = pthread_cond_init(&sem->cond, NULL);
    return SEM_ERROR_CODE(scode);
}

int
gp_semaphore_close(gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode, scode2;

    scode = pthread_cond_destroy(&sem->cond);
    scode2 = pthread_mutex_destroy(&sem->mutex);
    if (scode == 0)
	scode = scode2;
    return SEM_ERROR_CODE(scode);
}

int
gp_semaphore_wait(gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode, scode2;

    scode = pthread_mutex_lock(&sem->mutex);
    if (scode != 0)
	return SEM_ERROR_CODE(scode);
    while (sem->count == 0) {
        scode = pthread_cond_wait(&sem->cond, &sem->mutex);
        if (scode != 0)
	    break;
    }
    if (scode == 0)
	--sem->count;
    scode2 = pthread_mutex_unlock(&sem->mutex);
    if (scode == 0)
	scode = scode2;
    return SEM_ERROR_CODE(scode);
}

int
gp_semaphore_signal(gp_semaphore * sema)
{
    pt_semaphore_t * const sem = (pt_semaphore_t *)sema;
    int scode, scode2;

    scode = pthread_mutex_lock(&sem->mutex);
    if (scode != 0)
	return SEM_ERROR_CODE(scode);
    if (sem->count++ == 0)
	scode = pthread_cond_signal(&sem->cond);
    scode2 = pthread_mutex_unlock(&sem->mutex);
    if (scode == 0)
	scode = scode2;
    return SEM_ERROR_CODE(scode);
}


/* Monitor supports enter/leave semantics */

/*
 * We need PTHREAD_MUTEX_RECURSIVE behavior, but this isn't totally portable
 * so we implement it in a more portable fashion, keeping track of the
 * owner thread using 'pthread_self()'
 */
typedef struct gp_pthread_recursive_s {
    pthread_mutex_t mutex;	/* actual mutex */
    pthread_t	self_id;	/* owner */
} gp_pthread_recursive_t;

uint
gp_monitor_sizeof(void)
{
    return sizeof(gp_pthread_recursive_t);
}

int
gp_monitor_open(gp_monitor * mona)
{
    pthread_mutex_t * const mon = &((gp_pthread_recursive_t *)mona)->mutex;
    int scode;

    if (!mona)
	return -1;		/* monitors are not movable */
    ((gp_pthread_recursive_t *)mona)->self_id = 0;	/* Not valid unless mutex is locked */
    scode = pthread_mutex_init(mon, NULL);
    return SEM_ERROR_CODE(scode);
}

int
gp_monitor_close(gp_monitor * mona)
{
    pthread_mutex_t * const mon = &((gp_pthread_recursive_t *)mona)->mutex;
    int scode;

    scode = pthread_mutex_destroy(mon);
    return SEM_ERROR_CODE(scode);
}

int
gp_monitor_enter(gp_monitor * mona)
{
    pthread_mutex_t * const mon = (pthread_mutex_t *)mona;
    int scode;

    if ((scode = pthread_mutex_trylock(mon)) == 0) {
	((gp_pthread_recursive_t *)mona)->self_id = pthread_self();
	return SEM_ERROR_CODE(scode);
    } else {
	if (pthread_equal(pthread_self(),((gp_pthread_recursive_t *)mona)->self_id))
	    return 0;
	else {
	    /* we were not the owner, wait */
    scode = pthread_mutex_lock(mon);
	    ((gp_pthread_recursive_t *)mona)->self_id = pthread_self();
    return SEM_ERROR_CODE(scode);
}
    }
}

int
gp_monitor_leave(gp_monitor * mona)
{
    pthread_mutex_t * const mon = (pthread_mutex_t *)mona;
    int scode;

    scode = pthread_mutex_unlock(mon);
    ((gp_pthread_recursive_t *)mona)->self_id = 0;	/* Not valid unless mutex is locked */
    return SEM_ERROR_CODE(scode);
}


/* --------- Thread primitives ---------- */

/*
 * In order to deal with the type mismatch between our thread API, where
 * the starting procedure returns void, and the API defined by pthreads,
 * where the procedure returns void *, we need to create a wrapper
 * closure.
 */
typedef struct gp_thread_creation_closure_s {
    gp_thread_creation_callback_t proc;  /* actual start procedure */
    void *proc_data;			/* closure data for proc */
} gp_thread_creation_closure_t;

/* Wrapper procedure called to start the new thread. */
static void *
gp_thread_begin_wrapper(void *thread_data /* gp_thread_creation_closure_t * */)
{
    gp_thread_creation_closure_t closure;

    closure = *(gp_thread_creation_closure_t *)thread_data;
    free(thread_data);
    DISCARD(closure.proc(closure.proc_data));
    return NULL;		/* return value is ignored */
}

int
gp_create_thread(gp_thread_creation_callback_t proc, void *proc_data)
{
    gp_thread_creation_closure_t *closure =
	(gp_thread_creation_closure_t *)malloc(sizeof(*closure));
    pthread_t ignore_thread;
    pthread_attr_t attr;
    int code;

    if (!closure)
	return_error(gs_error_VMerror);
    closure->proc = proc;
    closure->proc_data = proc_data;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    code = pthread_create(&ignore_thread, &attr, gp_thread_begin_wrapper,
			  closure);
    if (code) {
	free(closure);
	return_error(gs_error_ioerror);
    }
    return 0;
}
