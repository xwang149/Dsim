#!/bin/bash
# echo "run with FIFO on case1"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=0 --output=../results/datsim_output_case1_FIFO.log > ../results/case1_FIFO.output
# ./simviz.py -e ../results/datsim_output_case1_FIFO.log

# echo "run with priority on case1"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=2 --output=../results/datsim_output_case1_P.log > ../results/case1_P.output
# ./simviz.py -e ../results/datsim_output_case1_P.log

# echo "run with utility on case1"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=1 --output=../results/datsim_output_case1_PU.log > ../results/case1_PU.output
# ./simviz.py -e ../results/datsim_output_case1_PU.log 

echo "run with utility on case1"
./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=1 --output=../results/datsim_output_case1_U.log > ../results/case1_U.output
./simviz.py -e ../results/datsim_output_case1_U.log 

# echo "run with FIFO on case2"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=0 --output=../results/datsim_output_case2_FIFO.log > ../results/case2_FIFO.output
# ./simviz.py -e ../results/datsim_output_case2_FIFO.log

# echo "run with priority on case2"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=2 --output=../results/datsim_output_case2_P.log > ../results/case2_P.output
# ./simviz.py -e ../results/datsim_output_case2_P.log

# echo "run with utility on case2"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=1 --output=../results/datsim_output_case2_PU.log > ../results/case2_PU.output
# ./simviz.py -e ../results/datsim_output_case2_PU.log 

# echo "run with FIFO on case3"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=0 --output=../results/datsim_output_case3_FIFO.log > ../results/case3_FIFO.output
# ./simviz.py -e ../results/datsim_output_case3_FIFO.log

# echo "run with priority on case3"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=2 --output=../results/datsim_output_case3_P.log > ../results/case3_P.output
# ./simviz.py -e ../results/datsim_output_case3_P.log

# echo "run with utility on case3"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=1 --output=../results/datsim_output_case3_PU.log > ../results/case3_PU.output
# ./simviz.py -e ../results/datsim_output_case3_PU.log 

# echo "run with FIFO on case4"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case4.conf --sched-policy=0 --output=../results/datsim_output_case4_FIFO.log > ../results/case4_FIFO.output
# ./simviz.py -e ../results/datsim_output_case4_FIFO.log

# echo "run with priority on case4"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case4.conf --sched-policy=2 --output=../results/datsim_output_case4_P.log > ../results/case4_P.output
# ./simviz.py -e ../results/datsim_output_case4_P.log

# echo "run with utility on case4"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case4.conf --sched-policy=1 --output=../results/datsim_output_case4_PU.log > ../results/case4_PU.output
# ./simviz.py -e ../results/datsim_output_case4_PU.log 

# echo "run with FIFO on case5"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case5.conf --sched-policy=0 --output=../results/datsim_output_case5_FIFO.log > ../results/case5_FIFO.output
# ./simviz.py -e ../results/datsim_output_case5_FIFO.log

# echo "run with priority on case5"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case5.conf --sched-policy=2 --output=../results/datsim_output_case5_P.log > ../results/case5_P.output
# ./simviz.py -e ../results/datsim_output_case5_P.log

# echo "run with utility on case5"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobs.trace --synch=1 --trans-limit=trans-limit-case5.conf --sched-policy=1 --output=../results/datsim_output_case5_PU.log > ../results/case5_PU.output
# ./simviz.py -e ../results/datsim_output_case5_PU.log 




echo "-----------------------------------------------------------------------------------------------"
echo "run with 2 times job sizes"

# echo "run with FIFO on case1"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=0 --output=../results/datsim_output_case1_FIFO_x2.log > ../results/case1_FIFO_x2.output
# ./simviz.py -e ../results/datsim_output_case1_FIFO_x2.log

# echo "run with priority on case1"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=2 --output=../results/datsim_output_case1_P_x2.log > ../results/case1_P_x2.output
# ./simviz.py -e ../results/datsim_output_case1_P_x2.log

# echo "run with utility on case1"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case1.conf --sched-policy=1 --output=../results/datsim_output_case1_PU_x2.log > ../results/case1_PU_x2.output
# ./simviz.py -e ../results/datsim_output_case1_PU_x2.log 


# echo "run with FIFO on case2"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=0 --output=../results/datsim_output_case2_FIFO_x2.log > ../results/case2_FIFO_x2.output
# ./simviz.py -e ../results/datsim_output_case2_FIFO_x2.log

# echo "run with priority on case2"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=2 --output=../results/datsim_output_case2_P_x2.log > ../results/case2_P_x2.output
# ./simviz.py -e ../results/datsim_output_case2_P_x2.log

# echo "run with utility on case2"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case2.conf --sched-policy=1 --output=../results/datsim_output_case2_PU_x2.log > ../results/case2_PU_x2.output
# ./simviz.py -e ../results/datsim_output_case2_PU_x2.log 

# echo "run with FIFO on case3"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=0 --output=../results/datsim_output_case3_FIFO_x2.log > ../results/case3_FIFO_x2.output
# ./simviz.py -e ../results/datsim_output_case3_FIFO_x2.log

# echo "run with priority on case3"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=2 --output=../results/datsim_output_case3_P_x2.log > ../results/case3_P_x2.output
# ./simviz.py -e ../results/datsim_output_case3_P_x2.log

# echo "run with utility on case3"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case3.conf --sched-policy=1 --output=../results/datsim_output_case3_PU_x2.log > ../results/case3_PU_x2.output
# ./simviz.py -e ../results/datsim_output_case3_PU_x2.log 

# echo "run with FIFO on case4"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case4.conf --sched-policy=0 --output=../results/datsim_output_case4_FIFO_x2.log > ../results/case4_FIFO_x2.output
# ./simviz.py -e ../results/datsim_output_case4_FIFO_x2.log

# echo "run with priority on case4"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case4.conf --sched-policy=2 --output=../results/datsim_output_case4_P_x2.log > ../results/case4_P_x2.output
# ./simviz.py -e ../results/datsim_output_case4_P_x2.log

# echo "run with utility on case4"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case4.conf --sched-policy=1 --output=../results/datsim_output_case4_PU_x2.log > ../results/case4_PU_x2.output
# ./simviz.py -e ../results/datsim_output_case4_PU_x2.log 

# echo "run with FIFO on case5"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case5.conf --sched-policy=0 --output=../results/datsim_output_case5_FIFO_x2.log > ../results/case5_FIFO_x2.output
# ./simviz.py -e ../results/datsim_output_case5_FIFO_x2.log

# echo "run with priority on case5"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case5.conf --sched-policy=2 --output=../results/datsim_output_case5_P_x2.log > ../results/case5_P_x2.output
# ./simviz.py -e ../results/datsim_output_case5_P_x2.log

# echo "run with utility on case5"
# ./datsim --codes-config=datsim_wan_three_site.conf --jobtrace=jobsx2.trace --synch=1 --trans-limit=trans-limit-case5.conf --sched-policy=1 --output=../results/datsim_output_case5_PU_x2.log > ../results/case5_PU_x2.output
# ./simviz.py -e ../results/datsim_output_case5_PU_x2.log 