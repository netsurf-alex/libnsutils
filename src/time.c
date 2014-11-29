/*
 * Copyright 2014 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnsutils.
 *
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

/**
 * \file
 * Time operation implementation
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#if (defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) && (defined _POSIX_MONOTONIC_CLOCK)) || defined(__OpenBSD__)
#include <time.h>
#elif defined(__riscos)
#include <oslib/os.h>
#elif defined(__MACH__)
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#elif defined(__amigaos4__)
#include <assert.h>
#include <proto/timer.h>
#else
#include <sys/time.h>
#endif
#include "nsutils/time.h"

/* exported interface documented in nsutils/time.h */
nsuerror nsu_getmonotonic_ms(uint64_t *current_out)
{
    uint64_t current;
    static uint64_t prev = 0; /* previous time so we never go backwards */

#if (defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) && (defined _POSIX_MONOTONIC_CLOCK)) || defined(__OpenBSD__)
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    current = (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000);
#elif defined(__riscos)
    os_t time;

    time = os_read_monotonic_time();
    current = time * 10;
#elif defined(__MACH__)
    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);

    current = (mts.tv_sec * 1000) + (mts.tv_nsec / 1000000);
#elif defined(__amigaos4__)
    struct TimeVal tv;

    /* NB: The calling task must already have opened timer.device
     * and obtained the interface.
     */
    assert(ITimer != NULL);

    ITimer->GetUpTime(&tv);
    current = (tv.Seconds * 1000) + (tv.Microseconds / 1000);
#else
#warning "Using dodgy gettimeofday() fallback"
    /** \todo Implement this properly! */
    struct timeval tv;

    gettimeofday(&tv, NULL);

    current = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif

    /* ensure time never goes backwards */
    if (current > prev) {
        *current_out = current;
        prev = current;
    } else {
        /** \todo is 1ms really correct or can we calculate a delta going forwards? */
        prev += 1;
        *current_out = prev;
    }
    return NSUERROR_OK;
}
