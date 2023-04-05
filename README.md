This is the code repo for the ICAPS 2022 paper:  **Conflict-Based Search for the Virtual Network Embedding Problem** by Yi Zheng, Srivatsan Ravi, Erik Kline, Sven Koenig, T. K. Satish Kumar. 

The code was built based on the ViNE-Yard simulator from the paper: 

*Chowdhury, N.M.K., Rahman, M.R. and Boutaba, R., 2009, April. Virtual network embedding with coordinated node and link mapping. In IEEE INFOCOM 2009 (pp. 783-791). IEEE.* 

 The code requires the external libraries Boost (https://www.boost.org/). An easy way to install it on Ubuntu: 

```
sudo apt update
sudo apt install libboost-all-dev
```

After installing Boost, you can compile the source code with CMake: 

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make
```

Then, you can run the code:

- Online experiment sample:

```
./VNE_CBS_Sim 20 ../test_vnr/online_test ../sub.txt my_test.out mapping.txt 1.0
```

- Parameters:
  - 20 (num_vnr):  the number of VNRs to use in experiment. 1 for offline setting. 
  - ../test_vnr/online_test (vnr_folder): the path to the folder of VNRs.
  - ../sub.txt (substrate_net_file): the path to the substrate network file. 
  - my_test.out (output_file): the path to the file that contains the output information, such as cost and runtime.
  - mapping.txt (mapping_output_file): the path to the file that contains the VNE mappings for each VNR.
  - 1.0 (suboptimality factor): the cost bound of the solution. Values > 1.0 trade solution cost for runtime.
- Offline experiment sample:

```
./VNE_CBS_Sim 1 ../test_vnr/offline_test/0 ../sub.txt my_test.out mapping.txt 1.0
```

We provided one substrate network file "sub.txt" and a nubmber of VNRs in "test_vnr" folder.

**The format of an output file:** 

First line: the command line used to generate the results.

Then each line: <VNR_id>, <arrive_or_depart_timestep>, <VNR_duration>, <arrive_or_depart>, <accepted_or_not>, <total_revenue>, <total_cost>, <num_CBS_nodes>, <runtime_seconds>.

**The format of a substrate network file:** 

First line: num_node, num_edges.

Then node lines: <node_loc_x>, <noc_loc_y>, <CPU_capacity>

Then edge lines: <from_node>, <to_node>, <bandwidth_capacity>, <delay_> (not used)

**The format of a VNR file:**

First line: <num_node>, <num_edge>, <split_or_not> (not used), <arrive_timestep>, <VNR_duration>, <topology_num> (not used), <max_distance>

Then node lines: <node_loc_x>, <noc_loc_y>, <CPU_requirement>

Then edge lines: <from_node>, <to_node>, <bandwidth_requirement>, <delay_requirement> (not used)