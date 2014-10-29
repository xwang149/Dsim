#include "util.h"
#include "datsim_types.h"
#include "lp_source_host.h"
#include "lp_source_router.h"
#include "lp_dest_host.h"

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <ross-types.h>
#include "ross.h"

#include "codes/model-net.h"
#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"

/* define state*/
typedef struct dest_host_state dest_host_state;
struct dest_host_state {
    int  total_processed;
    double data_download_time; /*in sec*/
    tw_stime start_ts;    /* time that we started sending requests */
    tw_stime end_ts;      /* time that last request finished */
};

/* ROSS expects four functions per LP:
 * - an LP initialization function, called for each LP
 * - an event processing function
 * - a *reverse* event processing function (rollback), and
 * - a finalization/cleanup function when the simulation ends
 */
static void lpf_dest_host_init(dest_host_state * ns, tw_lp * lp);
static void lpf_dest_host_event(dest_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void lpf_dest_host_rev_event(dest_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void lpf_dest_host_finalize(dest_host_state * ns, tw_lp * lp);

/*event handlers*/
static void handle_kick_off_event(dest_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_data_download_event(dest_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);

/*msg senders*/
static void send_received_notification(char* work_id, tw_lp *lp);

/* set up the function pointers for ROSS, as well as the size of the LP state
 * structure (NOTE: ROSS is in charge of event and state (de-)allocation) */
tw_lptype dest_host_lp = {
     (init_f) lpf_dest_host_init,
     (pre_run_f) NULL,
     (event_f) lpf_dest_host_event,
     (revent_f) lpf_dest_host_rev_event,
     (final_f) lpf_dest_host_finalize,
     (map_f) codes_mapping,
     sizeof(dest_host_state),
};

/*start implementations*/
const tw_lptype* dest_host_get_lp_type()
{
    return(&dest_host_lp);
}

void init_dest_host() {

    return;
}


void register_lp_dest_host()
{
    /* lp_type_register should be called exactly once per process per
     * LP type */
    lp_type_register("dest_host", dest_host_get_lp_type());
}

void lpf_dest_host_init(
    dest_host_state * ns,
    tw_lp * lp)
{
    tw_event *e;
    datsim_msg *m;
    tw_stime kickoff_time;

    memset(ns, 0, sizeof(*ns));

    /* skew each kickoff event slightly to help avoid event ties later on */
    kickoff_time = g_tw_lookahead + tw_rand_unif(lp->rng); ;

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
void lpf_dest_host_event(
    dest_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
   switch (m->event_type)
    {
        case KICK_OFF:
            handle_kick_off_event(ns, b, m, lp);
            break;
        case SEND_REQ:
        	handle_data_download_event(ns, b, m, lp);
            break;
        default:
	    printf("\ndest_host Invalid message type %d from %lu\n", m->event_type, m->src);
        break;
    }
}

/* reverse event processing entry point
 * - simply forward the message to the appropriate handler */
void lpf_dest_host_rev_event(
    dest_host_state * cs,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    return;
}

/* once the simulation is over, do some output */
void lpf_dest_host_finalize(
    dest_host_state * ns,
    tw_lp * lp)
{
    ns->end_ts = tw_now(lp);
    double makespan = ns_to_s(ns->end_ts - ns->start_ts);
    double download_rate = ns->data_download_time / makespan;

    printf("[dest_host][%lu]start_time=%lf, end_time=%lf, makespan=%lf, processed=%d, data_download_rate=%lf\n",
            lp->gid,
            ns_to_s(ns->start_ts),
            ns_to_s(ns->end_ts),
            makespan,
            ns->total_processed,
            download_rate
            );
    return;
}

/* handle initial event (initialize job submission) */
void handle_kick_off_event(
    dest_host_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
	printf("[%lf][dest_host][%lu]Start serving...\n", now_sec(lp), lp->gid);
    return;
}

/* input downloaded -> start run command*/
void handle_data_download_event(dest_host_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp) {
    if (strlen(m->object_id)>0) {
        char* jobid = m->object_id;
        Job* job = g_hash_table_lookup(job_map, jobid);
        job->stats.end = now_sec(lp);
        double data_move_time_sec = job->stats.end - job->stats.start;
        strcpy(job->state, "done");
        fprintf(event_log, "%lf;dest_host;%lu;JD;jobid=%s download_size=%llu download_time=%lf\n",
        		now_sec(lp),
                lp->gid,
                jobid,
                job->inputsize,
                data_move_time_sec);
        ns->data_download_time += data_move_time_sec;
        ns->total_processed += 1;
        send_received_notification(jobid, lp);
    }
}

void send_received_notification(char* job_id, tw_lp *lp) {

    tw_event *e;
    datsim_msg *msg;
    tw_lpid dest_id = get_source_router_lp_id();
    e = codes_event_new(dest_id, ns_tw_lookahead, lp);
    msg = tw_event_data(e);
    msg->event_type = RECEIVE_ACK;
    msg->src = lp->gid;
    msg->next_hop = get_source_host_lp_id();
    strcpy(msg->object_id, job_id);
    tw_event_send(e);
    return;
}
