#!/bin/bash
echo "run with FIFO on case1"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=0 --output=../results/datsim_output_case1_FIFO.log > ../results/case1_FIFO.output
./simviz.py -e ../results/datsim_output_case1_FIFO.log

echo "run with priority on case1"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=2 --output=../results/datsim_output_case1_P.log > ../results/case1_P.output
./simviz.py -e ../results/datsim_output_case1_P.log

echo "run with utility on case1"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=1 --output=../results/datsim_output_case1_U.log > ../results/case1_U.output
./simviz.py -e ../results/datsim_output_case1_U.log 


echo "run with FIFO on case2"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=0 --output=../results/datsim_output_case2_FIFO.log > ../results/case2_FIFO.output
./simviz.py -e ../results/datsim_output_case2_FIFO.log

echo "run with priority on case2"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=2 --output=../results/datsim_output_case2_P.log > ../results/case2_P.output
./simviz.py -e ../results/datsim_output_case2_P.log

echo "run with utility on case2"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=1 --output=../results/datsim_output_case2_U.log > ../results/case2_U.output
./simviz.py -e ../results/datsim_output_case2_U.log 

echo "run with FIFO on case3"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=0 --output=../results/datsim_output_case3_FIFO.log > ../results/case3_FIFO.output
./simviz.py -e ../results/datsim_output_case3_FIFO.log

echo "run with priority on case3"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=2 --output=../results/datsim_output_case3_P.log > ../results/case3_P.output
./simviz.py -e ../results/datsim_output_case3_P.log

echo "run with utility on case3"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=1 --output=../results/datsim_output_case3_U.log > ../results/case3_U.output
./simviz.py -e ../results/datsim_output_case3_U.log 