#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "glib.h"

#include "util.h"
#include "datsim_types.h"

GHashTable *job_map=NULL;

int main(
    int argc,
    char **argv)
{
    /*parse workload file and init job_map*/
    job_map = parse_jobtrace("test.trace");



}
