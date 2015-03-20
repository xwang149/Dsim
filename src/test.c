#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "glib.h"

#include "datsim_types.h"

int W = 5;
int C_max = 10;
double MAX_NUM = 9999999999999999;
double now = 1367619918230900;
static GQueue *job_queue;

void index_of(int func, int results[], double delta[]);

static void parse_dest_bandwidth(Job *job, char *bandwidth_filename){
    FILE *f;
    char line[2048];
    f = fopen(bandwidth_filename, "r");
    if (f == NULL) {
    	perror(bandwidth_filename);
    	exit(1);
    }
    char dest[MAX_LENGTH_ID];
    while ( fgets ( line, sizeof(line), f ) != NULL ){ /* read a line */
    	gchar ** parts = NULL;
    	g_strstrip((gchar*)line);
    	parts = g_strsplit((gchar*)line,"=", 5);
        strcpy(dest, parts[0]);
        if (strcmp(job->dest_host, dest) == 0)
        	job->bandwidth = atoi(parts[1])*1024*1024;
        memset(line, 0, sizeof(line));
    }
    fclose(f);
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
    //print_job(jb);
    return jb;
}

static void read_jobtrace(char* filename){
	FILE *f;
	char line[2048];
	f = fopen(filename, "r");
	if (f == NULL) {
		perror(filename);
		exit(1);
	}

	while ( fgets ( line, sizeof(line), f ) != NULL ){ /* read a line */
		Job* jb=NULL;
	    jb = parse_job_by_trace((gchar*)line);
	    memset(line, 0, sizeof(line));
	    g_queue_push_tail(job_queue, jb);
	}
    fclose(f);
}

static void init_conn(int conn[]){
	//evenly spread C_max connections to jobs in Window
	int base = C_max / W;
	int addition = C_max % W;
	for (int i = 0; i < W; i++) {
		if (i < addition)
			conn[i] = base+1;
		else
			conn[i] = base;
	}
}

static void calc_utility_matrix(double U[][C_max+1]){
	for(int c=1; c<=C_max;c++){
		for (int i = 0; i<W; i++){
	    	Job* job = g_queue_peek_nth(job_queue, i);
	    	assert(job);
	    	double t_est = (double) job->inputsize/(c*job->bandwidth/C_max);
	    	double resp = (double) (now - job->stats.created)/1000000 + t_est;
	    	double ideal = (double) job->inputsize / (job->bandwidth/5) + 0.01;
            double deadtime = 10*ideal;
            if (resp < deadtime)
            	U[i][c] = 100*(deadtime - resp)/deadtime;
            else
            	U[i][c] = 0;
            //printf("t_est=%12f;resp=%12f;ideal=%12f;dead=%12f;U=%12f\n",t_est,resp,ideal,deadtime,U[i][c]);
            printf("%12f", U[i][c]);
		}
		printf("\n");
	}

}

static void calc_delta(double leftDelta[], double rightDelta[], int conn[], double U[][C_max+1]){
	for(int i=0; i<W; i++){
		if (conn[i] == 0)
			leftDelta[i] = MAX_NUM;
		else if (conn[i] == C_max)
			rightDelta[i] = 0 - MAX_NUM;
		else{
			leftDelta[i] = U[i][conn[i]] - U[i][conn[i]-1];
			rightDelta[i] = U[i][conn[i]+1] - U[i][conn[i]];
		}
	}
}

static void find_solution(int sol[], double leftDelta[], double rightDelta[]){
	int max_results[2], min_results[2];
	int hasSol = 0;
	//find max and second max index of leftDelta
	index_of(1, max_results, rightDelta);
	//find min and second min index of leftDelta
	index_of(0, min_results, leftDelta);

	//find a solution that increment utility
	if (max_results[0] != min_results[0]) {
		double increment = rightDelta[max_results[0]]-leftDelta[min_results[0]];
		if (increment > 0){
			sol[0] = max_results[0];
			sol[1] = min_results[0];
			hasSol = 1;
		}
	}
	else {
		double i1= rightDelta[max_results[0]]-leftDelta[min_results[1]];
		double i2= rightDelta[max_results[1]]-leftDelta[min_results[0]];
		if (i1 >= i2 && i1 > 0){
			if (i1 > 0){
				sol[0] = max_results[0];
				sol[1] = min_results[1];
				hasSol = 1;
			}
		}
		else if (i2>i1 && i2 > 0){
				sol[0] = max_results[1];
				sol[1] = min_results[0];
				hasSol = 1;
		}
	}

	if (hasSol != 1){
		sol[0] = -1;
		sol[1] = -1;
	}
}

void index_of(int func, int results[], double delta[]) {
	int first,second;
	if (func == 1) { //find max of rightDelta
		if (delta[0] < delta[1]) {
		    second = 0; first = 1;
		} else {
		    second = 1; first = 0;
		}
		for(int i = 2; i < W; i++){
			if (delta[first] < delta[i]){
				second = first;
				first = i;
			}
			else if (delta[second] < delta[i])
				second = i;
		}
	}
	if (func == 0) { //find min of leftDelta
		if (delta[0] > delta[1]) {
		    second = 0; first = 1;
		} else {
		    second = 1; first = 0;
		}
		for( int i=2; i<W;i++){
			if (delta[first] > delta[i]){
				second = first;
				first = i;
			}
			else if (delta[second] > delta[i])
				second = i;
		}
	}
	results[0] = first;
	results[1] = second;
}

static void display_sol(int conn[]){
	for( int i=0; i<W; i++){
		printf("jobid=%d;connection=%d\n",i, conn[i]);
	}
}

static void display_delta(double leftDelta[], double rightDelta[]){
	printf("   leftDelta  rightDelta\n");
	for(int i=0; i<W; i++){
		printf("%12f%12f\n",leftDelta[i], rightDelta[i]);
	}
}


int main(
    int argc,
    char **argv)
{
    job_queue= g_queue_new();
    /*parse workload file and put job in queue*/
    read_jobtrace("test.trace");

    int conn[W];
    double U[W][C_max+1], leftDelta[W], rightDelta[W];

    //calculate utility matrix
    calc_utility_matrix(U);
    printf("Done calc utility...\n");

    //initial connection assignment
    init_conn(conn);
    printf("Done init assignment...\n");

    int DONE = 0;
    while (!DONE) {
//      display_sol(conn);
    	calc_delta(leftDelta, rightDelta, conn, U);
//    	display_delta(leftDelta, rightDelta);
    	int sol[2];
    	//find a solution that can increase utility
    	find_solution(sol, leftDelta, rightDelta);

    	if (sol[0]==-1 || sol[1]==-1){
    		//if cannot find a solution, stop
    		DONE = 1;
    	}
    	else {
    		//if finds a solution, update connection assignment
    		conn[sol[0]]++;
    		conn[sol[1]]--;
    	}
    }
    printf("Results:\n");
    display_sol(conn);

}
