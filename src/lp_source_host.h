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

extern int sched_policy; //0: round-robin, 1: data-aware-best-fit, 2: data-aware-greedy

#endif	/* LP_SOURCE_HOST_H */
