#include <stdlib.h>	/* exit, malloc, realloc, free */
#include <stdio.h>	/* fopen, fgetc, fputs, fwrite */
#include <string.h>
#include <stdint.h>

#include "util.h"

int net_id = 0;
FILE *event_log = NULL;

float fraction = 1.0;
int trans_limit = 0;
int sched_policy = 0;
int MAX_CONN = 0;
int WINDOW = 0;

static Job* parse_job_by_trace(gchar *line);
static void parse_dest_bandwidth(Job *job, char *bandwidth_filename);

/*serve as MAX epoch time, Sat, 20 Nov 2286 17:46:39 GMT*/
double kickoff_epoch_time = 9999999999999999;

/* convert ns to seconds */
tw_stime ns_to_s(tw_stime ns)
{
    return(ns / (1000.0 * 1000.0 * 1000.0));
}

/* convert seconds to ns */
tw_stime s_to_ns(tw_stime ns)
{
    return(ns * (1000.0 * 1000.0 * 1000.0));
}

tw_stime ns_to_us(tw_stime ns)
{
    return(ns / (1000.0));
}

/* convert us to ns  */
tw_stime us_to_ns(tw_stime ns)
{
    return(ns * 1000.0);
}

/*convert eporch time to simulation time*/
tw_stime etime_to_stime(double etime) {
    return etime - kickoff_epoch_time;
}


static void free_key(gpointer data) {
	//not used for now (key is part of the data thus can be freed with the data)
}

static void free_value(gpointer data) {
	free(data);
}


void print_job(Job* job) {
    printf("jobid=%s;source=%s;destination=%s;queued=%f;size=%lu;state=%s",
        job->id, 
        job->source_host,
        job->dest_host,
        job->stats.created,
        job->inputsize,
        job->state
    );
    printf("\n");
}

void print_key_value(gpointer key, gpointer value, gpointer user_data)
{
    if (strcmp((char*)user_data, "job_map")==0) {
        Job* job = (Job*)value;
        print_job(job);
    }
}

void display_hash_table(GHashTable *table, char* name)
{
    printf("displaying hash table:\n");
    g_hash_table_foreach(table, print_key_value, name);
}

GHashTable* parse_jobtrace(char* jobtrace_path) {
    FILE *f;
    char line[MAX_LEN_TRACE_LINE];
    f = fopen(jobtrace_path, "r");
    if (f == NULL) {
    	perror(jobtrace_path);
    	exit(1);
    }
    
    GHashTable *job_map = NULL;
    job_map =  g_hash_table_new_full(g_str_hash, g_str_equal, free_key, free_value);
    printf("[source_host]parsing job trace ...\n");
    
    while ( fgets ( line, sizeof(line), f ) != NULL ){ /* read a line */
        Job* jb=NULL;   
        jb = parse_job_by_trace((gchar*)line);
        memset(line, 0, sizeof(line));
        g_hash_table_insert(job_map, jb->id, jb);
    }
    
    printf("[source_host]parsing job trace ... done: %u jobs parsed\n", g_hash_table_size(job_map));
    fclose(f);
    return job_map;
}

GHashTable* init_tasktable() {
    GHashTable *task_map = NULL;
    task_map =  g_hash_table_new_full(g_str_hash, g_str_equal, free_key, free_value);
    return task_map;
}

Job* parse_job_by_trace(gchar* line) {
    Job* jb=NULL;
    jb = malloc(sizeof(Job));
    memset(jb, 0, sizeof(Job));
    gchar ** parts = NULL;
    g_strstrip(line);
    parts = g_strsplit(line, ";", 30);
    int i;
    for (i = 0; i < 30; i++) {
        if (!parts[i])
            break;
        gchar **pair = g_strsplit(parts[i], "=", 2);
        char* key = pair[0];
        char* val = pair[1];
        char *endptr;
        if (strcmp(key, "id")==0) {
            strcpy(jb->id, val);
        } else if (strcmp(key, "submission")==0) {
            jb->stats.created = atol(val);
        } else if (strcmp(key, "source")==0) {
        	strcpy(jb->source_host, val);
        } else if (strcmp(key, "dest")==0) {
        	strcpy(jb->dest_host, val);
        } else if (strcmp(key, "size")==0) {
        	jb->inputsize = strtoll(val, &endptr, 10);
        } else if (strcmp(key, "deadline")==0) {
        	if (val != NULL)
        		jb->deadline = atol(val);
        }
    }
    parse_dest_bandwidth(jb, "bw-3sites.conf");
    strcpy(jb->state, "raw");
    double ideal = (double) jb->inputsize / (jb->bandwidth/MAX_CONN) + 0.01;
    jb->t_dead = 10 * ideal;
    if (jb->t_dead < 300)
    	jb->t_dead = 300;
    //print_job(jb);
    return jb;
}

static void parse_dest_bandwidth(Job *job, char *bandwidth_filename){
    FILE *f;
    char line[MAX_LEN_TRACE_LINE];
    f = fopen(bandwidth_filename, "r");
    if (f == NULL) {
    	perror(bandwidth_filename);
    	exit(1);
    }
    uint64_t total_band;
    char dest[MAX_LENGTH_ID];
    while ( fgets ( line, sizeof(line), f ) != NULL ){ /* read a line */
    	gchar ** parts = NULL;
    	g_strstrip((gchar*)line);
    	parts = g_strsplit((gchar*)line,"=", 5);
    	total_band += atoi(parts[1])*1024*1024;
        strcpy(dest, parts[0]);
        if (strcmp(job->dest_host, dest) == 0)
        	job->bandwidth = atoi(parts[1])*1024*1024;
        memset(line, 0, sizeof(line));
    }
    fclose(f);
}


GHashTable* parse_trans_limit(char* trans_limit_filename) {
    FILE *f;
    char line[MAX_LEN_TRACE_LINE];
    f = fopen(trans_limit_filename, "r");
    if (f == NULL) {
    	perror(trans_limit_filename);
    	exit(1);
    }
    GHashTable *limit_map = NULL;
    limit_map =  g_hash_table_new_full(g_str_hash, g_str_equal, free_key, free_value);
    printf("[source_host]parsing trans-limit from %s ...\n", trans_limit_filename);
    while ( fgets ( line, sizeof(line), f ) != NULL ){ /* read a line */
    	Trans_Limit* tl=NULL;
    	tl = malloc(sizeof(Trans_Limit));
    	memset(tl, 0, sizeof(Trans_Limit));
        gchar ** parts = NULL;
        g_strstrip((gchar*)line);
        parts = g_strsplit((gchar*)line,"=", 5);
        strcpy(tl->host_id, parts[0]);
        tl->trans_limit = atoi(parts[1]);
        memset(line, 0, sizeof(line));
        g_hash_table_insert(limit_map, tl->host_id, tl);

        if (strcmp(parts[0],"0")==0){
        	MAX_CONN = atoi(parts[1]);
        	WINDOW = MAX_CONN / 2;
        }
    }
    fclose(f);
    return limit_map;
}

