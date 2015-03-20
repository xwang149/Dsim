#!/usr/bin/env python
'''simviz.py analyzes and visualizes the output of awesim'''

import datetime
import json
import math
import matplotlib.pyplot as plt
from pylab import *
import numpy as np
import sys
import time
from sets import Set
from optparse import OptionParser

max_uscore = 10
pre_deadline = 3

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
        parts = line.split(";")
        if parts[1] == "dest_host" and parts[3] == "TD":
            dest_host_id = parts[2]
            vals = {}
            for i in range(4, 13):
                p = parts[i].split("=")
                vals[p[0]] = p[1]
            jobid = vals["jobid"]
            job_stats[jobid] = {}
            job_stats[jobid]["dest_id"] = dest_host_id
            job_stats[jobid]["inputsize"] = float(vals["download_size"])
            job_stats[jobid]["submit"] = float(vals["submit_time"])
            job_stats[jobid]["start"] = float(vals["start_time"])
            job_stats[jobid]["end"] = float(vals["end_time"])
            job_stats[jobid]["deadline"] = float(vals["deadline"])
            job_stats[jobid]["bandwidth"] = float(vals["bandwidth"])
            job_stats[jobid]["t_dead"] = float(vals["t_dead"])
            job_stats[jobid]["wait"] = job_stats[jobid]["start"] - job_stats[jobid]["submit"]
            job_stats[jobid]["resp"] = job_stats[jobid]["end"] - job_stats[jobid]["submit"]
            if (job_stats[jobid]["inputsize"] <= 1073741824):
                job_stats[jobid]["idealtime"] =  max(job_stats[jobid]["t_dead"]/20, 300/20)
            else:
                job_stats[jobid]["idealtime"] =  max(job_stats[jobid]["t_dead"]/40, 300/40)
            job_stats[jobid]["slowdown"] = (job_stats[jobid]["end"] - job_stats[jobid]["start"]) / job_stats[jobid]["idealtime"]
            job_stats[jobid]["resp_slow"] = job_stats[jobid]["resp"] / job_stats[jobid]["idealtime"]
            event_type=parts[3]
            if event_type in ["TS", "JD"]:
                t = float(parts[0])
                p = parts[6].split("=")
                d = parts[5].split("=")
                concur = int(p[1])
                dest_id = d[1]
                if not trans_stats.has_key(dest_id):
                    trans_stats[dest_id] = []
                trans_stats[dest_id].append([t, concur])
    return trans_stats, job_stats

def calc_utility(job_stats):
    for jobid in job_stats.keys():
        # if job_stats[jobid]["deadline"] == 0:
            if (job_stats[jobid]["resp"] < job_stats[jobid]["idealtime"]):
                y1 = 100
            elif (job_stats[jobid]["resp"] < job_stats[jobid]["t_dead"]):
                y1 = 100*(job_stats[jobid]["t_dead"] - job_stats[jobid]["resp"])/job_stats[jobid]["t_dead"]
#                y1 = 80 + (10*job_stats[jobid]["t_dead"] - 20*job_stats[jobid]["resp"])/(0.4*job_stats[jobid]["t_dead"])
#            elif (job_stats[jobid]["resp"] < job_stats[jobid]["t_dead"]):
#                y1 = 160* (job_stats[jobid]["t_dead"] - job_stats[jobid]["resp"])/job_stats[jobid]["t_dead"]
                # utilityscore.append(y1)
            else:
                # utilityscore.append(0)
                y1 = 0
            job_stats[jobid]["utilityscore"] = y1
        # else:
        #     deadtime = max(5, (max_uscore - pre_deadline)*job_stats[jobid]["idealtime"]+job_stats[jobid]["deadline"]-job_stats[jobid]["submit"])
        #     if(job_stats[jobid]["resp"] <= job_stats[jobid]["deadline"]-job_stats[jobid]["submit"]):
        #         # utilityscore.append(100.0)
        #         job_stats[jobid]["utilityscore"] = 100
        #     elif (job_stats[jobid]["resp"] < deadtime):
        #         y2 = 100*(deadtime - job_stats[jobid]["resp"])/(deadtime-job_stats[jobid]["deadline"]+job_stats[jobid]["submit"])
        #         # utilityscore.append(y2)
        #         job_stats[jobid]["utilityscore"] = y2
        #     else:
        #         # utilityscore.append(0)
        #         job_stats[jobid]["utilityscore"] = 0
    return job_stats

def calc_job_stats(job_stats):
    job_metrics= {}
    job_metrics["makespan"] = 0
    job_metrics["wait_t"] = []
    job_metrics["resp"] = []
    job_metrics["slowdown"] = []
    job_metrics["resp_slow"] = []
    job_metrics["total_data"] = 0
    job_metrics["inputsize"] = []
    job_metrics["utility"] = []
    
    for key, stats in job_stats.iteritems():
        if stats["end"] > job_metrics["makespan"]:
            job_metrics["makespan"] = stats["end"]
        job_metrics["total_data"] += stats["inputsize"]
        job_metrics["wait_t"].append(stats["wait"])
        job_metrics["resp"].append(stats["resp"])
        job_metrics["slowdown"].append(stats["slowdown"])
        job_metrics["resp_slow"].append(stats["resp_slow"])
        job_metrics["inputsize"].append(stats["inputsize"])
        job_metrics["utility"].append(stats["utilityscore"])
    return job_metrics
   
def show_job_metrics(job_metrics, filename):
    outfile = open(filename, "a")
    # outfile.write("avg_wait, avg_resp, avg_trans, avg_slow, avg_cont, acc_utility, agg_utility, host\n")

    avg_wait = sum(job_metrics["wait_t"]) / len(job_metrics["wait_t"])
    avg_resp = sum(job_metrics["resp"]) / len(job_metrics["resp"])
    avg_trans = sum(job_metrics["resp"])-sum(job_metrics["wait_t"]) / len(job_metrics["resp"])
    avg_cont = sum(job_metrics["slowdown"]) / len(job_metrics["slowdown"])
    avg_slow = sum(job_metrics["resp_slow"]) / len(job_metrics["resp_slow"])
    agg_utility = np.dot(job_metrics["utility"], job_metrics["resp"])
    acc_utility = sum(job_metrics["utility"])
    sys_throughput = job_metrics["total_data"] / job_metrics["makespan"] / 1048576
    print "[source_host]\n"
    print "       avg_wait(sec)       avg_resp(sec)      avg_trans(sec)            avg_slow            avg_cont            acc_util            agg_util\n"
    print "%20f%20f%20f%20f%20f%20f%20f\n" % (avg_wait, avg_resp, avg_trans,avg_slow, avg_cont, acc_utility, agg_utility)
    line = "%s,%s,%s,%s,%s,%s,%s,%s\n" % (avg_wait, avg_resp, avg_trans, avg_slow, avg_cont, acc_utility, agg_utility, 0)
    outfile.write(line)
    outfile.close()

def calc_dest_metrics(job_stats):
    dest_metrics= {}
    for key, stats in job_stats.iteritems():
        dest_id = stats["dest_id"]
        if not dest_metrics.has_key(dest_id):
            dest_metrics[dest_id] = {}
            dest_metrics[dest_id]["makespan"] = 0
            dest_metrics[dest_id]["wait_t"] = []
            dest_metrics[dest_id]["resp"] = []
            dest_metrics[dest_id]["runtime"] = []
            dest_metrics[dest_id]["slowdown"] = []
            dest_metrics[dest_id]["resp_slow"] = []
            dest_metrics[dest_id]["total_data"] = 0
            dest_metrics[dest_id]["inputsize"] = []
            dest_metrics[dest_id]["bandwidth"] = 0
            dest_metrics[dest_id]["utilityscore"] = []
        if stats["end"] > dest_metrics[dest_id]["makespan"]:
            dest_metrics[dest_id]["makespan"] = stats["end"]
        dest_metrics[dest_id]["total_data"] += stats["inputsize"]
        dest_metrics[dest_id]["wait_t"].append(stats["wait"])
        dest_metrics[dest_id]["resp"].append(stats["resp"])
        dest_metrics[dest_id]["runtime"].append(stats["resp"] - stats["wait"])
        dest_metrics[dest_id]["slowdown"].append(stats["slowdown"])
        dest_metrics[dest_id]["resp_slow"].append(stats["resp_slow"])
        dest_metrics[dest_id]["inputsize"].append(stats["inputsize"])
        dest_metrics[dest_id]["bandwidth"] = stats["bandwidth"]
        dest_metrics[dest_id]["utilityscore"].append(stats["utilityscore"])
    return dest_metrics

def show_dest_metrics(dest_metrics, filename):
    sorted_keys = dest_metrics.keys()
    sorted_keys.sort()
    outfile = open(filename, "a")
    # outfile.write("avg_wait, avg_resp, avg_trans, avg_slow, avg_cont, acc_utility, agg_utility, band, host\n")
    for key in sorted_keys:
        avg_wait = sum(dest_metrics[key]["wait_t"]) / len(dest_metrics[key]["wait_t"])
        avg_resp = sum(dest_metrics[key]["resp"]) / len(dest_metrics[key]["resp"])
        avg_trans = sum(dest_metrics[key]["resp"])-sum(dest_metrics[key]["wait_t"]) / len(dest_metrics[key]["resp"])
        avg_cont = sum(dest_metrics[key]["slowdown"]) / len(dest_metrics[key]["slowdown"])
        avg_slow = sum(dest_metrics[key]["resp_slow"]) / len(dest_metrics[key]["resp_slow"])
        band = dest_metrics[key]["bandwidth"] / 1048576
        # avg_utility = sum(dest_metrics[key]["utilityscore"]) / len(dest_metrics[key]["utilityscore"])
        agg_utility = np.dot(dest_metrics[key]["utilityscore"], dest_metrics[key]["resp"])
        acc_utility = sum(dest_metrics[key]["utilityscore"])
        dest_throughput = dest_metrics[key]["total_data"] / dest_metrics[key]["makespan"] / 1048576
        print "[dest_host][%s]\n" % (key)
        print "       avg_wait(sec)       avg_resp(sec)      avg_trans(sec)            avg_slow            avg_cont            acc_util            agg_util           bandwidth\n"
        print "%20f%20f%20f%20f%20f%20f%20f%20f\n" % (avg_wait, avg_resp,avg_trans, avg_slow, avg_cont, acc_utility, agg_utility,  band)
        line = "%s,%s,%s,%s,%s,%s,%s,%s,%s\n" % (avg_wait, avg_resp,avg_trans, avg_slow, avg_cont, acc_utility, agg_utility, band, key)
        outfile.write(line)
    outfile.close() 

def generate_csv(job_stats, table_name):

    outfile = open(table_name, "w")

    # outfile.write("jobid, dest_id, size, submit_time, start_time, end_time, deadline, bandwidth, wait, resp, run, ideal, slow, resp_slow, utility\n")
    
    for jobid in job_stats.keys():    
        dest_id = job_stats[jobid]["dest_id"] 
        size = job_stats[jobid]["inputsize"]
        submit_time = job_stats[jobid]["submit"]
        start_time = job_stats[jobid]["start"]
        end_time = job_stats[jobid]["end"]
        deadline = job_stats[jobid]["deadline"]
        bandwidth = job_stats[jobid]["bandwidth"]
        wait = job_stats[jobid]["wait"]
        resp = job_stats[jobid]["resp"]
        run = job_stats[jobid]["end"] - job_stats[jobid]["start"]
        ideal = job_stats[jobid]["idealtime"]
        slow = job_stats[jobid]["slowdown"]
        resp_slow = job_stats[jobid]["resp_slow"]
        utility = job_stats[jobid]["utilityscore"]
        
        line = ("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n") % (jobid, dest_id, size, submit_time, start_time, end_time, deadline, bandwidth, wait, resp, run, ideal, slow, resp_slow, utility)
        outfile.write(line)   
    outfile.close()



def plot_concurrency(trans_stats, filename, fig_num):
    plt.figure(fig_num)
    sorted_keys = trans_stats.keys()
    sorted_keys.sort()
    for key in sorted_keys:
        max_time = int(math.ceil(max(trans_stats[key])[0]))
        trans_stats[key].sort()
        step = []
        concur = []
        for i in xrange(0, max_time+1):
            step.append(i)
            pos = 0
            c = 0
            while (pos < len(trans_stats[key])):
                if (trans_stats[key][pos][0] > i):
                    break
                c = trans_stats[key][pos][1]
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
        log_size = math.log(stats["inputsize"])
        load_stats[dest_id].append([stats["submit"], log_size])
    plt.figure(fig_num)
    sorted_keys = load_stats.keys()
    sorted_keys.sort()
    for key in sorted_keys:
        max_time = int(math.ceil(max(load_stats[key])[0]))
        load_stats[key].sort()
        step = []
        size = []
        for i in xrange(0, len(load_stats[key])):
            step.append(load_stats[key][i][0])
            size.append(load_stats[key][i][1])
        plt.plot(step, size, "o", label='dest_'+key)
    plt.legend()
    plt.savefig(filename)

def prepare_bins(dest_metrics):
    resp_time = {}
    run_time = {}
    utility = {}
    dest_ids = dest_metrics.keys()
    for dest_id in dest_ids:
        if not resp_time.has_key(dest_id):
            resp_time[dest_id] = []
        dest_metrics[dest_id]["resp"].sort()
        resp_time[dest_id] = dest_metrics[dest_id]["resp"]
        if not run_time.has_key(dest_id):
            run_time[dest_id] = []
        dest_metrics[dest_id]["runtime"].sort()
        run_time[dest_id] = dest_metrics[dest_id]["runtime"]
        if not utility.has_key(dest_id):
            utility[dest_id] = []
        dest_metrics[dest_id]["utilityscore"].sort()
        utility[dest_id] = dest_metrics[dest_id]["utilityscore"]
    return resp_time, run_time, utility

cdfyticks = [0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]

def plot_cdf(bins, fig_num, label_name, filename):
    output = {}
    #points = range(0, 601)
    sorted_keys = bins.keys()
    sorted_keys.sort()
    points = []
    max_k = []
    for k in sorted_keys:
        m = math.ceil(max(bins[k]))
        max_k.append(m)
    maximum = int(max(max_k)*1.1)    
    points = [ x for x in range(0, maximum) ]
    fig = plt.figure(fig_num)
    ax = fig.add_subplot(111)
    ax.set_xlabel(label_name)
    ax.set_ylabel('percentage')
    for k in sorted_keys:
        for m in points:
            num = 0.0
            if len(bins[k]) == 0:
                continue
            for j in bins[k]:
                if j <= m:
                    num += 1
            if not output.has_key(k):
                output[k] = []
            frag = num/len(bins[k])
            output[k].append(frag)
        plt.plot(points, output[k], "-", label = "dest_"+k)
    ind = np.arange(len(output.values()[0]), step=1)
    plt.xlim(0, max(ind))
    incre = (max(ind) -  min(ind))/20
    plt.xticks(np.arange(min(ind), max(ind), incre))
    plt.ylim(ymin=0, ymax=1.1)
    plt.yticks(cdfyticks)
    plt.legend()
    plt.savefig(filename)

if __name__ == "__main__":
    p = OptionParser()
    p.add_option("-e", "--eventlog", dest = "eventlog", type = "string", 
                    help = "path of event log file")
    
    (opts, args) = p.parse_args()
    
    if not opts.eventlog:
        print "please specify path of event log file (-e)"
        p.print_help()
        exit()
    
    outfile = opts.eventlog[0:opts.eventlog.rfind(".")]
    trans_stats, job_stats = parse_event_log(opts.eventlog)
    job_stats = calc_utility(job_stats)

    job_metrics = calc_job_stats(job_stats)
    show_job_metrics(job_metrics, "../results/results_source.csv")

    generate_csv(job_stats, outfile+"_jobstats.csv")

    dest_metrics  = calc_dest_metrics(job_stats)
    show_dest_metrics(dest_metrics, "../results/results_dest.csv")

    # plot_concurrency(trans_stats, opts.eventlog, 1)
    # plot_job_load(job_stats, "../results/job_load.png", 2)

    # resp_time, run_time, utility = prepare_bins(dest_metrics)
    # plot_cdf(resp_time, 3, "resp_time", outfile+"_resp_cdf.png")
    # plot_cdf(run_time, 4, "run_time", outfile+"_runtime_cdf.png")
    # plot_cdf(utility, 5, "utility", outfile+"_utility_cdf.png")
    exit()
