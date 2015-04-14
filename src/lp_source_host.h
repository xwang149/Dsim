/*
 * File:   lp_source_host.h
 * Author: xwang
 *
 * Created on Oct. 22, 2014, 10:50 AM
 */

#ifndef LP_SOURCE_HOST_H
#define	LP_SOURCE_HOST_H

#include "glib.h"
#include "ross.h"


extern void init_source_host();
extern void register_lp_source_host();
extern tw_lpid get_source_host_lp_id();

extern int sched_policy; //0: FCFS, 1: Utility, 2: Weighted Queuing
extern int MAX_CONN;
extern int WINDOW;

#endif	/* LP_SOURCE_HOST_H */
