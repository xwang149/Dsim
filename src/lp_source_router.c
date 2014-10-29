#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "ross.h"
#include "glib.h"

#include "lp_source_router.h"
#include "util.h"
#include "datsim_types.h"

#include "codes/model-net.h"
#include "codes/codes.h"
#include "codes/codes_mapping.h"
#include "codes/configuration.h"
#include "codes/lp-type-lookup.h"


/* define state*/
typedef struct source_router_state source_router_state;
/* this struct serves as the ***persistent*** state of the LP representing the
 * server in question. This struct is setup when the LP initialization function
 * ptr is called */
struct source_router_state {
    long long size_download;
    double time_download;
    tw_stime start_ts;    /* time that we started sending requests */
    tw_stime end_ts;      /* time that last request finished */
};


/* ROSS expects four functions per LP:
 * - an LP initialization function, called for each LP
 * - an event processing function
 * - a *reverse* event processing function (rollback), and
 * - a finalization/cleanup function when the simulation ends
 */
static void lpf_source_router_init(source_router_state * ns, tw_lp * lp);
static void lpf_source_router_event(source_router_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void lpf_source_router_rev_event(source_router_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void lpf_source_router_finalize(source_router_state * ns, tw_lp * lp);

/*event handlers*/
static void handle_kick_off_event(source_router_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_data_send_req_event(source_router_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);
static void handle_data_receive_ack_event(source_router_state * ns, tw_bf * b, datsim_msg * m, tw_lp * lp);

/* set up the function pointers for ROSS, as well as the size of the LP state
 * structure (NOTE: ROSS is in charge of event and state (de-)allocation) */
tw_lptype source_router_lp = {
     (init_f) lpf_source_router_init,
     (pre_run_f) NULL,
     (event_f) lpf_source_router_event,
     (revent_f) lpf_source_router_rev_event,
     (final_f) lpf_source_router_finalize,
     (map_f) codes_mapping,
     sizeof(source_router_state),
};

/*start implementations*/

const tw_lptype* source_router_get_lp_type()
{
    return(&source_router_lp);
}

void register_lp_source_router()
{
    /* lp_type_register should be called exactly once per process per
     * LP type */
    lp_type_register("source_router", source_router_get_lp_type());
}

tw_lpid get_source_router_lp_id() {
    tw_lpid rtn_id;
    codes_mapping_get_lp_id("SOURCE_ROUTER", "source_router", NULL, 1, 0, 0, &rtn_id);
    return rtn_id;
}

void lpf_source_router_init(
	source_router_state * ns,
    tw_lp * lp)
{
    tw_event *e;
    datsim_msg *m;
    tw_stime kickoff_time;

    memset(ns, 0, sizeof(*ns));
    /* skew each kickoff event slightly to help avoid event ties later on */
    kickoff_time = 0.00;
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
void lpf_source_router_event(
    source_router_state * ns,
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
            handle_data_send_req_event(ns, b, m, lp);
            break;
        case RECEIVE_ACK:
            handle_data_receive_ack_event(ns, b, m, lp);
            break;
        default:
	    printf("\n source_router Invalid message type %d \n", m->event_type);
        break;
    }
}

/* reverse event processing entry point
 * - simply forward the message to the appropriate handler */
void lpf_source_router_rev_event(
    source_router_state * qs,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    return;
}

/* once the simulation is over, do some output */
void lpf_source_router_finalize(
    source_router_state * ns,
    tw_lp * lp)
{
    ns->end_ts = tw_now(lp);
    printf("[source_router][%lu]start_time=%lf;end_time=%lf, makespan=%lf, data_forward_size=%llu\n",
        lp->gid,
        ns->start_ts,
        ns->end_ts,
        ns->end_ts - ns->start_ts,
        ns->size_download
        );
    return;
}



/* handle initial event (initialize job submission) */
void handle_kick_off_event(
    source_router_state * qs,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    printf("[%lf][source_router][%lu]Start serving...\n", now_sec(lp), lp->gid);
    return;
}

void handle_data_send_req_event(
    source_router_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
     datsim_msg m_remote;
     tw_lpid dest_id = m->next_hop;
     m_remote.event_type = SEND_REQ;
     m_remote.src = lp->gid;
     m_remote.last_hop = m->src;
     strcpy(m_remote.object_id, m->object_id);
     m_remote.size = m->size;
     model_net_event(net_id, "download", dest_id, m->size, 0.0, sizeof(datsim_msg), (const void*)&m_remote, 0, NULL, lp);
     ns->size_download +=m->size;

    return;
}

void handle_data_receive_ack_event(
    source_router_state * ns,
    tw_bf * b,
    datsim_msg * m,
    tw_lp * lp)
{
    tw_event *e;
    datsim_msg *msg;
    tw_lpid dest_id = m->next_hop;
    e = codes_event_new(dest_id, ns_tw_lookahead, lp);
    msg = tw_event_data(e);
    msg->event_type = RECEIVE_ACK;
    msg->src = lp->gid;
    msg->last_hop = m->src;
    msg->size = m->size;
    strcpy(msg->object_id, m->object_id);
    tw_event_send(e);
    return;
}
