#!/bin/bash
echo "run without transfer limitation"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-no-limit.conf --output=../results/datsim_output_simple_schedular.log > ../results/simple_schedular.output
#./simviz.py -e ../results/datsim_output_simple_schedular.log

echo "run with transfer limitation of 1:1:1"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-1:1:1.conf --output=../results/datsim_output_limit_1:1:1.log > ../results/limit_1:1:1.output
#./simviz.py -e ../results/datsim_output_limit_1:1:1.log

echo "run with transfer limitation of 1:4:1"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-1:4:1.conf --output=../results/datsim_output_limit_1:4:1.log > ../results/limit_1:4:1.output
#./simviz.py -e ../results/datsim_output_limit_1:4:1.log