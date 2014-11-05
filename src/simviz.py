#!/usr/bin/env python
'''simviz.py analyzes and visualizes the output of awesim'''

import datetime
import json
import math
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
from sets import Set
from optparse import OptionParser
#from Carbon.Aliases import true

#color_list = ['b', 'r', 'k', 'g', 'm', 'y']
#stage_list = ["prep", "derep", "screen", "fgs", "uclust", "blat"]
#cmd_list = ["prep", "drpl", "scrn", "gncl", "clst", "sims", "rsch", "rcls", "rsim", "annt"]  #order matters (MG-RAST production pipeline)

#DEFAULT_TOTALWORK = 1
#CLIENT_QUOTA = 200

def parse_event_log(filename):
    '''parse event log for job stats'''
    
    job_stats = {}
    trans_stats = {}
    
    wlf = open(filename, "r")
    
    for line in wlf:
        line = line.strip('\n')
        line = line.strip('\r')
        if len(line)==0:
            continue
        parts = line.split(";");
        if parts[1] == "dest_host" and parts[3] == "TD":
            dest_host_id = parts[2]
            vals = {}
            for i in range(4, 9):
                p = parts[i].split("=");
                vals[p[0]] = p[1]
            jobid = vals["jobid"]
            job_stats[jobid] = {}
            job_stats[jobid]["dest_id"] = dest_host_id
            job_stats[jobid]["inputsize"] = float(vals["download_size"])
            job_stats[jobid]["submit"] = float(vals["submit_time"])
            job_stats[jobid]["start"] = float(vals["start_time"])
            job_stats[jobid]["end"] = float(vals["end_time"])
            job_stats[jobid]["wait"] = job_stats[jobid]["start"] - job_stats[jobid]["submit"]
            job_stats[jobid]["resp"] = job_stats[jobid]["end"] - job_stats[jobid]["submit"]
            job_stats[jobid]["runtime"] = job_stats[jobid]["end"] - job_stats[jobid]["start"]
            job_stats[jobid]["slowdown"] = float(job_stats[jobid]["resp"]) / job_stats[jobid]["runtime"]        
        elif parts[1] == "source_host":
            event_type=parts[3]
            if event_type in ["TS", "JD"]:
                t = float(parts[0])
                p = parts[6].split("=")
                d = parts[5].split("=")
                concur = int(p[1])
                dest_id = d[1]
                if not trans_stats.has_key(dest_id):
                    trans_stats[dest_id] = {}
                    trans_stats[dest_id]["time"] = []
                    trans_stats[dest_id]["concurrency"] = []
                trans_stats[dest_id]["time"].append(t)
                trans_stats[dest_id]["concurrency"].append(concur)
            
    return trans_stats, job_stats

def calc_job_stats(job_stats):
    job_metrics= {}
    job_metrics["makespan"] = 0
    job_metrics["wait_t"] = []
    job_metrics["resp"] = []
    job_metrics["slowdown"] = []
    job_metrics["total_data"] = 0
    job_metrics["inputsize"] = []
    
    for key, stats in job_stats.iteritems():
        if stats["end"] > job_metrics["makespan"]:
            job_metrics["makespan"] = stats["end"]
        job_metrics["total_data"] += stats["inputsize"]
        job_metrics["wait_t"].append(stats["wait"])
        job_metrics["resp"].append(stats["resp"])
        job_metrics["slowdown"].append(stats["slowdown"])
        job_metrics["inputsize"].append(stats["inputsize"])
        
    return job_metrics
   
def show_job_metrics(job_metrics):
    avg_wait = sum(job_metrics["wait_t"]) / len(job_metrics["wait_t"])
    avg_resp = sum(job_metrics["resp"]) / len(job_metrics["resp"])
    avg_slow = sum(job_metrics["slowdown"]) / len(job_metrics["slowdown"])
    sys_throughput = job_metrics["total_data"] / job_metrics["makespan"] / 1000000
    print "[source_host]average waiting time (sec) = %f average response time (sec)=%f, avg slowdown =%f, sys througput (MB/sec)=%f\n" % (avg_wait, avg_resp, avg_slow, sys_throughput)

def calc_dest_metrics(job_stats):
    dest_metrics= {}
    for key, stats in job_stats.iteritems():
        dest_id = stats["dest_id"]
        if not dest_metrics.has_key(dest_id):
            dest_metrics[dest_id] = {}
            dest_metrics[dest_id]["makespan"] = 0
            dest_metrics[dest_id]["wait_t"] = []
            dest_metrics[dest_id]["resp"] = []
            dest_metrics[dest_id]["slowdown"] = []
            dest_metrics[dest_id]["total_data"] = 0
            dest_metrics[dest_id]["inputsize"] = []
        if stats["end"] > dest_metrics[dest_id]["makespan"]:
            dest_metrics[dest_id]["makespan"] = stats["end"]
        dest_metrics[dest_id]["total_data"] += stats["inputsize"]
        dest_metrics[dest_id]["wait_t"].append(stats["wait"])
        dest_metrics[dest_id]["resp"].append(stats["resp"])
        dest_metrics[dest_id]["slowdown"].append(stats["slowdown"])
        dest_metrics[dest_id]["inputsize"].append(stats["inputsize"])
    return dest_metrics

def show_dest_metrics(dest_metrics):
    for key, stats in dest_metrics.iteritems():
        avg_wait = sum(stats["wait_t"]) / len(stats["wait_t"])
        avg_resp = sum(stats["resp"]) / len(stats["resp"])
        avg_slow = sum(stats["slowdown"]) / len(stats["slowdown"])
        dest_throughput = stats["total_data"] / stats["makespan"] / 1000000
        print "[dest_host][%s]average waiting time (sec) = %f average response time (sec)= %f, avg slowdown = %f, sys througput (MB/sec)= %f\n" % (key, avg_wait, avg_resp, avg_slow, dest_throughput)

def plot_concurrency(trans_stats, filename, fig_num):
    #plt.plot(trans_stats["time"], trans_stats["concurrency"], "ro")
    #plt.xticks(trans_stats["time"], trans_stats["time"], rotation="vertical")
    #plt.margins(0.2)
    #plt.subplots_adjust(bottom=0.15)
    #plt.show()
    plt.figure(fig_num)
    for key, stats in trans_stats.iteritems():
        max_time = int(math.ceil(max(stats["time"])))
        max_conr = max(stats["concurrency"])
        step = []
        concur = []
        for i in xrange(0, max_time+1):
            step.append(i)
            pos = 0
            c = 0
            while (pos < len(stats["concurrency"])):
                if (stats["time"][pos] > i):
                    break
                c = stats["concurrency"][pos]
                pos += 1
            concur.append(c)
        plt.step(step, concur, label='dest_'+key)
        # plt.xlim(0, max_time)
        # plt.ylim(0, max_conr+1)
    plt.legend()
    outfile = filename[0:filename.rfind(".")]+'_concur.png'
    plt.savefig(outfile)
    #plt.show()

def plot_job_load(job_stats, filename, fig_num):
    load_stats = {}
    index = 10
    for key, stats in job_stats.iteritems():
        dest_id = stats["dest_id"]
        if not load_stats.has_key(dest_id):
            load_stats[dest_id] = []
            # load_stats[dest_id]["submit_time"] = []
            # load_stats[dest_id]["data_size"] = []
        #load_stats[dest_id]["submit_time"].append(stats["submit"])
        log_size = math.log(stats["inputsize"])
        #load_stats[dest_id]["data_size"].append(log_size)
        load_stats[dest_id].append([stats["submit"], log_size])
    plt.figure(fig_num)
    for key in load_stats:
    #     max_time = int(math.ceil(max(stats["submit_time"])))
    #     step = []
    #     size = []
    #     for i in xrange(0, max_time+1):
    #         step.append(i)
    #         pos = 0
    #         s = 0
    #         while (pos < len(stats["data_size"])):
    #             if (stats["submit_time"][pos] > i):
    #                 break
    #             s += stats["data_size"][pos]
    #             pos += 1
    #         size.append(s)
    #     plt.plot(step, size, label='dest_'+key)
        # plt.plot(stats["submit_time"], stats["data_size"], "o", label='dest_'+key)
        max_time = int(math.ceil(max(load_stats[key])[0]))
        load_stats[key].sort()
        step = []
        size = []
        for i in xrange(0, max_time+1):
            step.append(i)
            pos = 0
            s = 0
            while (pos < len(load_stats[key])):
                if(load_stats[key][pos][0] > i):
                    break
                s += load_stats[key][pos][1]
                pos += 1
            size.append(s)
        plt.plot(step, size, label='dest_'+key)
    plt.legend()
    outfile = filename[0:filename.rfind(".")]+'_load.png'
    plt.savefig(outfile)



    
if __name__ == "__main__":
    p = OptionParser()
    p.add_option("-e", "--eventlog", dest = "eventlog", type = "string", 
                    help = "path of event log file")
    
    (opts, args) = p.parse_args()
    
    if not opts.eventlog:
        print "please specify path of event log file (-e)"
        p.print_help()
        exit()
    
    trans_stats, job_stats = parse_event_log(opts.eventlog)
    job_metrics = calc_job_stats(job_stats)
    show_job_metrics(job_metrics)
    dest_metrics  = calc_dest_metrics(job_stats)
    show_dest_metrics(dest_metrics)
    plot_concurrency(trans_stats, opts.eventlog, 1)
    plot_job_load(job_stats, opts.eventlog, 2)
    exit()
