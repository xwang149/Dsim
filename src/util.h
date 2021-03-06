/* 
 * File:   util.h
 * Author: xtang
 *
 * Created on Oct 22, 2014, 11:10 AM
 */

#ifndef UTIL_H
#define	UTIL_H

#include "ross.h"
#include "glib.h"
#include "datsim_types.h"

#define DATA_AWARE = 1

#define MAX_LEN_TRACE_LINE 2048

#define Mega 1048576

typedef int bool;
#define True 1
#define False 0

#define now_sec(lp)  ns_to_s(tw_now(lp))

extern int net_id;

extern double kickoff_epoch_time;

//extern char worktrace_file_name[256];
extern char jobtrace_file_name[256];
extern char output_file_name[256];
extern float fraction;
extern char trans_limit_filename[256];

extern const char* ready_string;

extern FILE *event_log;

tw_stime etime_to_stime(double etime);

tw_stime ns_to_s(tw_stime ns);
tw_stime s_to_ns(tw_stime ns);
tw_stime ns_to_us(tw_stime ns);
tw_stime us_to_ns(tw_stime ns);

#define ns_tw_lookahead 1000000   /* 0.001s */

GHashTable* parse_jobtrace(char* jobtrace_path);
void display_hash_table(GHashTable *table, char* name);
GHashTable* parse_trans_limit(char* trans_limit_filename);
GHashTable* init_tasktable();

#endif	/* UTIL_H */

