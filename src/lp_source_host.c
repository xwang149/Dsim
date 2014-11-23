#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "ross.h"
#include "glib.h"

#include "lp_source_host.h"
#include "util.h"
#include "datsim_types.h"

#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"

GHashTable *job_map=NULL;
GHashTable *task_map=NULL;
GHashTable *limit_map=NULL;
static GQueue* job_queue;
//static GQueue* priority_queue[MAX_PRIORITY];
static GQueue* task_queue;

/* define state*/
typedef struct source_host_state source_host_state;
/* this struct serves as the ***persistent*** state of the LP representing the
 * source_host in question. This struct is setup when the LP initialization function
 * ptr is called */
struct source_host_state {
    int total_job;
    int concur_jobs;
    int trans_limit;
    long size_forward;
    tw_stime start_ts;    /* time that we started sending requests */
    tw_stime end_ts;      /* time that last request finished */
};

/* ROSS expects four functions per LP:
 * - an LP initialization function, called for each LP
 * - an event processing function
 * - a *reverse* event processing function (rollback), and
 * - a finalization/cleanup function when the simulation ends
 */
static void lpf_source_host_init(source_host_state * ns, tw_lp * lp);
static void lpf_source_host_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void lpf_source_host_rev_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void lpf_source_host_finalize(source_host_state * ns, tw_lp * lp);

/*event handlers*/
static void handle_kick_off_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_job_submit_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_schedule_req_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_job_ready_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_job_done_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);

/* supportive functions */
static void handle_priority_queue(tw_lp * lp);
static gint compare_function(gconstpointer a, gconstpointer b, gpointer data);
static void calc_priority(tw_lp * lp, Job *job);
static void handle_task_queue(Job* job);
static tw_lpid plan_dest(Job* job);
//static void display_queue(GQueue *queue);

/* set up the function pointers for ROSS, as well as the size of the LP state
 * structure (NOTE: ROSS is in charge of event and state (de-)allocation) */
tw_lptype source_host_lp = {
     (init_f) lpf_source_host_init,
     (pre_run_f) NULL,
     (event_f) lpf_source_host_event,
     (revent_f) lpf_source_host_rev_event,
     (final_f) lpf_source_host_finalize,
     (map_f) codes_mapping,
     sizeof(source_host_state),
};

/*start implementations*/

const tw_lptype* source_host_get_lp_type()
{
    return(&source_host_lp);
}

//static void delete_job_entry(gpointer key, gpointer user_data) {
//	g_hash_table_remove(job_map, (char*)key);
//	printf("[awe-server]remove job %s from job_map\n", (char*)key);
//}

static void reset_epoch_time() {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, job_map);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Job* job = (Job*)value;
        if (job->stats.created < kickoff_epoch_time) {
            kickoff_epoch_time = job->stats.created;
        }
    }
}


void init_source_host() {
    /*parse workload file and init job_map*/
    job_map = parse_jobtrace(jobtrace_file_name);
    reset_epoch_time();
    limit_map = parse_trans_limit(trans_limit_filename);
    task_map = init_tasktable();
    printf("[source_host]checking jobs...done");
    //display_hash_table(job_map, "job_map");
    printf("[source_host]total valid jobs: %d\n", g_hash_table_size (job_map));
}


void register_lp_source_host() {
    /* lp_type_register should be called exactly once per process per
     * LP type */
    lp_type_register("source_host", source_host_get_lp_type());
}

tw_lpid get_source_host_lp_id() {
    tw_lpid rtn_id;
    codes_mapping_get_lp_id("SOURCE_HOST", "source_host", NULL, 1, 0, 0, &rtn_id);
    return rtn_id;
}


void lpf_source_host_init(
	source_host_state * ns,
    tw_lp * lp)
{
    tw_event *e;
    datsim_msg *m;
    tw_stime kickoff_time;

    memset(ns, 0, sizeof(*ns));
    job_queue = g_queue_new();
    task_queue = g_queue_new();

    /* skew each kickoff event slightly to help avoid event ties later on */
    kickoff_time = 0;

    //set source host trans_limit
    char lpid[MAX_LENGTH_ID] = "0";
    Trans_Limit *tl = g_hash_table_lookup(limit_map, lpid);
    assert(tl);
    ns->trans_limit = tl->trans_limit;

    /* first create the event (time arg is an offset, not absolute time) */
    e = codes_event_new(lp->gid, kickoff_time, lp);
    /* after event is created, grab the allocated message and set msg-specific
     * data */
    m = tw_event_data(e);
    m->event_type = KICK_OFF;
    m->src = lp->gid;
    /* event is ready to be processed, send it off */
    tw_event_send(e);

    return;
}

/* event processing entry point
 * - simply forward the message to the appropriate handler */
void lpf_source_host_event(
	source_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    switch (m->event_type)
    {
        case KICK_OFF:
            handle_kick_off_event(ns, b, m, lp);
            break;
        case JOB_SUBMIT:
            handle_job_submit_event(ns, b, m, lp);
            break;
        case SCHED_REQ:
        	handle_schedule_req_event(ns, b, m, lp);
        	break;
        case JOB_READY:
        	handle_job_ready_event(ns, b, m, lp);
            break;
        case RECEIVE_ACK:
            handle_job_done_event(ns, b, m, lp);
            break;
        default:
            printf("\nsource_host Invalid message type %d from %lu\n", m->event_type, m->src);
            break;
    }
}

/* reverse event processing entry point
 * - simply forward the message to the appropriate handler */
void lpf_source_host_rev_event(
	source_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    return;
}

/* once the simulation is over, do some output */
void lpf_source_host_finalize(
	source_host_state * ns,
    tw_lp * lp)
{
    ns->end_ts = tw_now(lp);
    printf("[source_host][%lu]start_time=%lf;end_time=%lf, makespan=%lf, total_job=%d, total_forward_size=%lu\n",
        lp->gid,
        ns->start_ts,
        ns->end_ts,
        ns->end_ts - ns->start_ts,
        ns->total_job,
        ns->size_forward);
    return;
}


/* handle initial event (initialize job submission) */
void handle_kick_off_event(
	source_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    printf("%lf;source_host;%lu]Start serving\n", now_sec(lp), lp->gid);
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, job_map);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Job* job = (Job*)value;
        tw_event *e;
        datsim_msg *msg;
        tw_stime create_time, submit_time, deadline=0;
        create_time =  us_to_ns(etime_to_stime(job->stats.created));
        if (job->deadline !=0)
        	deadline =  us_to_ns(etime_to_stime(job->deadline));
        submit_time = create_time + ns_tw_lookahead;
        if (fraction > 0 && fraction < 1.0) {
        	submit_time = submit_time * fraction;
        	deadline = deadline * fraction;
        }
        job->stats.created = ns_to_s(create_time);
        if (job->deadline !=0)
        	job->deadline = ns_to_s(deadline);
        e = codes_event_new(lp->gid, submit_time, lp);
        msg = tw_event_data(e);
        msg->event_type = JOB_SUBMIT;
        strcpy(msg->object_id, job->id);
        tw_event_send(e);
    }
    return;
}

void handle_job_submit_event(
	source_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    char* job_id = m->object_id;
    Job* job = g_hash_table_lookup(job_map, job_id);
    assert(job);
    fprintf(event_log, "%lf;source_host;%lu;JQ;jobid=%s\n",
    		now_sec(lp), lp->gid, job_id);
    //check job priority and put job in proper queue, then request for scheduler
    g_queue_push_tail(job_queue, job->id);
    tw_event *e;
    datsim_msg *msg;
    e = codes_event_new(lp->gid, ns_tw_lookahead, lp);
    msg = tw_event_data(e);
    msg->event_type = SCHED_REQ;
    strcpy(msg->object_id, job->id);
    tw_event_send(e);
    return;
}

static void handle_schedule_req_event(
	    source_host_state * ns,
	    tw_bf * b,
	    datsim_msg * m,
	    tw_lp * lp)
{
	if (strcmp(m->object_id, "NAN") != 0) {
		fprintf(event_log, "%lf;source_host;%lu;WQ;jobid=%s\n",
				now_sec(lp), lp->gid, m->object_id);
	}

	if (sched_policy == 1){
		//reorder job queue by priority
		handle_priority_queue(lp);
	}

    //schedule policy
    if (!g_queue_is_empty(job_queue)){
    	char job_id[MAX_LENGTH_ID];
    	strcpy(job_id, g_queue_peek_head(job_queue));
	    Job* job = g_hash_table_lookup(job_map, job_id);
	    assert(job);
    	//check trans_limit
        Trans_Limit *tl = g_hash_table_lookup(limit_map, job->dest_host);
        assert(tl);
    	if (tl->concur_jobs < tl->trans_limit && ns->concur_jobs < ns->trans_limit){
    		//pop the first job from the queue
    	    g_queue_pop_head(job_queue);
    	    //pass ready job
    	    tw_event *e;
    	    datsim_msg *msg;
    	    e = codes_event_new(lp->gid, ns_tw_lookahead, lp);
    	    msg = tw_event_data(e);
    	    msg->event_type = JOB_READY;
    	    strcpy(msg->object_id, job->id);
    	    tw_event_send(e);
    	}
    }
    return;
}


void handle_job_ready_event(
    source_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    char* job_id = m->object_id;
    Job* job = g_hash_table_lookup(job_map, job_id);
    assert(job);
    fprintf(event_log, "%lf;source_host;%lu;JR;jobid=%s\n", now_sec(lp), lp->gid, m->object_id);

    //split job into sub tasks
    handle_task_queue(job);

    printf("[%lf][source_host][%lu][StartSending]dest_host=%s;jobid=%s;filesize=%lu\n",
    		now_sec(lp), lp->gid, job->dest_host, job->id, job->inputsize);
    Trans_Limit *tl = g_hash_table_lookup(limit_map, job->dest_host);
    assert(tl);
    tl->concur_jobs += 1;
    ns->concur_jobs += 1;
    fprintf(event_log, "%lf;source_host;%lu;TS;jobid=%s;dest=%s;concurency=%d;priority=%f\n",
    		now_sec(lp), lp->gid, m->object_id, tl->host_id,tl->concur_jobs, job->priority);
    job->stats.start = now_sec(lp);

    while (!g_queue_is_empty(task_queue)){
    	char task_id[MAX_LENGTH_ID];
    	strcpy(task_id, g_queue_pop_head(task_queue));
	    Task* task = g_hash_table_lookup(task_map, task_id);
	    assert(task);
	    //prepare to send the task to destination
	    datsim_msg m_remote;
	    tw_lpid dest_id = get_source_router_lp_id();
	    m_remote.event_type = SEND_REQ;
	    m_remote.src = lp->gid;
	    m_remote.next_hop = plan_dest(job);
        strcpy(m_remote.object_id, task->task_id);
        m_remote.size = task->tasksize;
    	task->state = 1;
    	task->stats.start = now_sec(lp);
    	model_net_event(net_id, "download", dest_id, task->tasksize, 0.0, sizeof(datsim_msg), (const void*)&m_remote, 0, NULL, lp);
    }
//    model_net_event(net_id, "download", dest_id, job->inputsize, 0.0, sizeof(datsim_msg), (const void*)&m_remote, 0, NULL, lp);

//    //request the next job
//    tw_event *e;
//    datsim_msg *msg;
//    e = codes_event_new(lp->gid, ns_tw_lookahead, lp);
//    msg = tw_event_data(e);
//    msg->event_type = SCHED_REQ;
//    strcpy(msg->object_id, "NAN");
//    tw_event_send(e);
    return;
}

void handle_job_done_event(source_host_state * ns,
        tw_bf * b,
        datsim_msg * m,
        tw_lp * lp)
{
    char *job_id = m->object_id;
    ns->concur_jobs -= 1;
    Job* job = g_hash_table_lookup(job_map, job_id);
    ns->total_job += 1;
    ns->size_forward += job->inputsize;
    Trans_Limit *tl = g_hash_table_lookup(limit_map, job->dest_host);
    assert(tl);
    tl->concur_jobs -= 1;
    fprintf(event_log, "%lf;source_host;%lu;JD;jobid=%s;dest=%s;concurency=%d\n",
    		now_sec(lp), lp->gid, job_id,tl->host_id, tl->concur_jobs);
    //request the next job
    tw_event *e;
    datsim_msg *msg;
    e = codes_event_new(lp->gid, ns_tw_lookahead, lp);
    msg = tw_event_data(e);
    msg->event_type = SCHED_REQ;
    strcpy(msg->object_id, "NAN");
    tw_event_send(e);
}

static void handle_priority_queue(tw_lp * lp)
{
	if (!g_queue_is_empty(job_queue)){
		//calculate priority for each job in the queue
		for (guint i=0; i< g_queue_get_length(job_queue); i++){
	    	char job_id[MAX_LENGTH_ID];
	    	strcpy(job_id, g_queue_peek_nth(job_queue, i));
		    Job* job = g_hash_table_lookup(job_map, job_id);
		    assert(job);
		    calc_priority(lp, job);
		}
		//sort the job according to its priority score
		g_queue_sort(job_queue, (GCompareDataFunc)compare_function, NULL);

//		for (guint i=0; i< g_queue_get_length(job_queue); i++){
//	    	char job_id[MAX_LENGTH_ID];
//	    	strcpy(job_id, g_queue_peek_nth(job_queue, i));
//		    Job* job = g_hash_table_lookup(job_map, job_id);
//		    assert(job);
//		    printf("[%lf][source_host][%lu][queue]%lf\n",now_sec(lp), lp->gid, job->priority);
//		}
	}
}

static gint compare_function(gconstpointer a, gconstpointer b, gpointer data)
{
	char job_id1[MAX_LENGTH_ID], job_id2[MAX_LENGTH_ID];
	strcpy(job_id1, (char*) a);
	strcpy(job_id2, (char*) b);
    Job* job1 = g_hash_table_lookup(job_map, job_id1);
    Job* job2 = g_hash_table_lookup(job_map, job_id2);
    assert(job1); assert(job2);
    if (job1->priority < job2->priority)
    	return 1;
    else if (job1->priority > job2->priority)
    	return -1;
    else
    	return 0;
}

static void calc_priority(tw_lp * lp, Job *job)
{
	int pri=0;
	double wait_time = now_sec(lp) - job->stats.created;
	double ideal_time = (double) job->inputsize / job->bandwidth + 3*ns_tw_lookahead;
	double frac = 1 - (1/(wait_time / ideal_time + 1));;
	if (job->deadline == 0){
		while (pri < MAX_PRIORITY && wait_time >= ideal_time) {
			pri++;
			wait_time = wait_time - ideal_time;
		}
		job->priority = pri + frac;
	}
	else {
		double stall_time = job->deadline - MAX_PRIORITY * ideal_time;
		while (pri < MAX_PRIORITY && wait_time >= ideal_time + stall_time) {
			pri++;
			wait_time = wait_time - ideal_time;
		}
		job->priority = pri + frac;
	}
}


static void handle_task_queue(Job* job)
{
	int threadNum  = 2 * (1 + (long) job->priority);
	if (threadNum > 2*(MAX_PRIORITY+1))	threadNum = 2*(MAX_PRIORITY+1);
	uint64_t size = job->inputsize;
	uint64_t chunk = job->inputsize / threadNum;
	for (int i=0; i < threadNum; i++){
		Task *task = NULL;
	    task = malloc(sizeof(Task));
	    memset(task, 0, sizeof(Task));
		task->tasksize = size - (threadNum-i-1)*chunk;
		size = size - task->tasksize;
		strcpy(task->job_id, job->id);
		sprintf(task->task_id, "%s_%d", job->id, i);
		task->state = 0;
		task->taskNum = i;
		job->task_states[i] = 0;
		g_hash_table_insert(task_map, task->task_id, task);
		g_queue_push_tail(task_queue, task->task_id);
	}
	//display_queue(task_queue, ">>>");
	job->num_tasks = threadNum;
	job->remain_tasks = threadNum;
}

static tw_lpid plan_dest(Job* job)
{
	char *endptr;
	return strtoll(job->dest_host, &endptr, 10);
}

//static void display_queue(GQueue *queue)
//{
//    int len = g_queue_get_length(queue);
//    int i = 0;
//
//    for (i = 0; i < len; i++) {
//        printf("%s ", g_queue_peek_nth(queue, i));
//    }
//    printf("\n");
//}
