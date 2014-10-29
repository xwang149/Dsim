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
static GQueue* work_queue;

/* define state*/
typedef struct source_host_state source_host_state;
/* this struct serves as the ***persistent*** state of the LP representing the
 * source_host in question. This struct is setup when the LP initialization function
 * ptr is called */
struct source_host_state {
    int total_job;
    long long size_forward;
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
static void handle_work_enqueue_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_work_done_event(source_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);

/* plan the destination */
static tw_lpid plan_dest(Job* job);

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
    //printf("[awe_server]checking jobs...done, %d invalid jobs removed\n", ct);
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
    work_queue = g_queue_new();

    /* skew each kickoff event slightly to help avoid event ties later on */
    kickoff_time = 0;

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
        case WORK_ENQUEUE:
        	handle_work_enqueue_event(ns, b, m, lp);
            break;
        case RECEIVE_ACK:
            handle_work_done_event(ns, b, m, lp);
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
    printf("[source_host][%lu]start_time=%lf;end_time=%lf, makespan=%lf, total_job=%d, total_forward_size=%llu\n",
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
        tw_stime submit_time;
        submit_time =  us_to_ns(etime_to_stime(job->stats.created)) + ns_tw_lookahead;
        if (fraction > 0 && fraction < 1.0) {
        	submit_time = submit_time * fraction;
        }
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
    fprintf(event_log, "%lf;source_host;%lu;JQ;jobid=%s destination=%s inputsize=%llu\n",
    		now_sec(lp), lp->gid, job_id, job->dest_host, job->inputsize);
    //put job in the queue
    g_queue_push_tail(work_queue, job->id);
    tw_event *e;
    datsim_msg *msg;
    e = codes_event_new(lp->gid, ns_tw_lookahead, lp);
    msg = tw_event_data(e);
    msg->event_type = WORK_ENQUEUE;
    strcpy(msg->object_id, job->id);
    tw_event_send(e);
    return;
}


void handle_work_enqueue_event(
    source_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    fprintf(event_log, "%lf;source_host;%lu;WQ;jobid=%s\n", now_sec(lp), lp->gid, m->object_id);
    //pop first job in the queue
    char job_id[MAX_LENGTH_ID];
    strcpy(job_id, g_queue_pop_head(work_queue));
    Job* job = g_hash_table_lookup(job_map, job_id);
    assert(job);

    //prepare to send the job to destination
    datsim_msg m_remote;
    tw_lpid dest_id = get_source_router_lp_id();
    m_remote.event_type = SEND_REQ;
    m_remote.src = lp->gid;
    m_remote.next_hop = plan_dest(job);
    strcpy(m_remote.object_id, job->id);
    m_remote.size = job->inputsize;
    printf("[%lf][source_host][%lu][StartSending]dest_host=%s;filesize=%llu\n", now_sec(lp), lp->gid, job->dest_host, job->inputsize);
    job->stats.start = now_sec(lp);
    model_net_event(net_id, "download", dest_id, job->inputsize, 0.0, sizeof(datsim_msg), (const void*)&m_remote, 0, NULL, lp);

    return;
}

void handle_work_done_event(source_host_state * ns,
        tw_bf * b,
        datsim_msg * m,
        tw_lp * lp)
{
    char *job_id = m->object_id;
    fprintf(event_log, "%lf;source_host;%lu;WD;jobid=%s\n", now_sec(lp), lp->gid, job_id);
    Job* job = g_hash_table_lookup(job_map, job_id);
    ns->total_job += 1;
    ns->size_forward += job->inputsize;

}

static tw_lpid plan_dest(Job* job)
{
	char *endptr;
	return strtoll(job->dest_host, &endptr, 10);
}
