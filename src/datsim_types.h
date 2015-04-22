/* 
 * File:   datsim_types.h
 * Author: xwang
 *
 * Created on Oct 22, 2014, 9:10 AM
 */

#ifndef DATSIM_TYPES_H
#define	DATSIM_TYPES_H

#include <stdint.h>

#define MAX_LENGTH_UUID 36
#define MAX_LENGTH_ID 45
#define MAX_LENGTH_STATE 15
#define MAX_LENGTH_GROUP 20
#define MAX_NAME_LENGTH_WKLD 512
#define MAX_IO_FILE_NUM 100
#define MAX_NUM_TASKS 30

#define TIMER_CHECKOUT_INTERVAL 100
#define MAX_NUM 9999999999999999
#define PREDEADLINE 3

//extern  GHashTable *work_map;
extern  GHashTable *job_map;
extern  GHashTable *task_map;
extern  GHashTable *limit_map;

/* common event, msg types*/

/* define event*/
typedef enum datsim_event_type datsim_event_type;
enum datsim_event_type
{
    KICK_OFF,    /* initial event */
    JOB_SUBMIT,  /*from initilized workload*/
    SCHED_REQ,   /*trigger scheduler iteration*/
    JOB_READY,   /*scheduler -> sender to start a data transfer job*/
    /*following are data related events*/
    SEND_REQ,  /* source_host -> router, router -> dest_host*/
    RECEIVE_ACK,  /* dest_host -> router, router ->  source_host*/
};

typedef struct datsim_msg datsim_msg;
struct datsim_msg {
    enum datsim_event_type event_type;
    tw_lpid src;          /* source of this request or ack */
    tw_lpid next_hop;          /* for fwd msg, next hop to forward */
    tw_lpid last_hop;          /* for fwd msg, last hop before forward */
    char object_id[MAX_LENGTH_ID];
    uint64_t size;  /*data size*/
    int incremented_flag; /* helper for reverse computation */
};

/* end of ross common msg types*/

/* data structure for DATSIM elements*/
typedef struct JobStat JobStat;
struct JobStat  {
    double created;
    double start;
    double end;
    uint64_t remained_size;
};

typedef struct Job Job;
struct Job {
    char id[MAX_LENGTH_ID];
    char source_host[MAX_NAME_LENGTH_WKLD];
    char dest_host[MAX_NAME_LENGTH_WKLD];
    uint64_t inputsize;
    uint64_t bandwidth;
    double t_dead;
    double deadline;
    double priority;
    int num_tasks;
    int remain_tasks;
    int task_states[MAX_NUM_TASKS];  /* 0=pending, 1=transferring, 2=completed*/
    char state[MAX_LENGTH_STATE];
    JobStat stats;
};

typedef struct TaskStat TaskStat;
struct TaskStat {
    double created;
    double start;
    double end;
};

typedef struct Task Task;
struct Task{
    char job_id[MAX_LENGTH_ID];
    char task_id[MAX_LENGTH_ID];
    char dest_host[MAX_NAME_LENGTH_WKLD];
    uint64_t tasksize;
    int taskNum;
    int num_tasks;
    int state;
    TaskStat stats;
};

typedef struct Trans_Limit Trans_Limit;
struct Trans_Limit {
	char host_id[MAX_LENGTH_ID];
	int trans_limit;
	int concur_jobs;
};

/* end of data structure for DATSIM elements*/

#endif	/* DATSIM_TYPES_H */


