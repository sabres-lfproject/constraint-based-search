# Conflict-Based Solver (cbs)

This is the code repo for the ICAPS 2022 paper:  [Conflict-Based Search for the Virtual Network Embedding Problem](https://ojs.aaai.org/index.php/ICAPS/article/view/19828) by Yi Zheng, Srivatsan Ravi, Erik Kline, Sven Koenig, T. K. Satish Kumar. 

The code was built based on the ViNE-Yard simulator from IEEE INFOCOM 2009 [Virtual Network Embedding with Coordinated Node and Link Mapping](https://ieeexplore.ieee.org/document/5061987) by N.M.K. Chowdhury, M.R. Rahman, and R. Boutaba.

## Dependencies

* c++
* cmake
* make
* libboost

### Debian/Ubuntu

```
sudo apt update
sudo apt install gcc g++ cmake make libboost-all-dev
```

### Development

* clang-format
* docker

## Building

```
mkdir build
cd build
cmake ..
make
```

## Running

We've provided one substrate network file `sub.txt` and example VNRs in the `test-vnr` directory.  Below are examples using the online and offline uses.

### Online experiment example

```
./build/cbs 20 test-vnr/online-test test-vnr/sub.txt test.out mapping.txt 1.0
```

Inputs:

>  1: 20 (`num_vnr`):  the number of VNRs to use in experiment. 1 for offline setting. 

>  2: test-vnr/online-test (`vnr_folder`): the path to the folder of VNRs.

>  3: test-vnr/sub.txt (`substrate_net_file`): the path to the substrate network file. 

>  4: test.out (`output_file`): the path to the file that contains the output information, such as cost and runtime.

>  5: mapping.txt (`mapping_output_file`): the path to the file that contains the VNE mappings for each VNR.

>  6: 1.0 (suboptimality factor): the cost bound of the solution. Values > 1.0 trade solution cost for runtime.


### Offline experiment example:

```
./build/cbs 1 test-vnr/offline-test/0 test-vnr/sub.txt test.out mapping.txt 1.0
```

### Substrate Network format

First line: num_node, num_edges.

Then node lines: <node_loc_x>, <noc_loc_y>, <CPU_capacity>

Then edge lines: <from_node>, <to_node>, <bandwidth_capacity>, <delay_> (not used)

The following is an example of 11 node, 22 edge topology.

We've formulated the SN with the notion that traffic originates at `0,1` coords (1,1 and 2,2) and sinks at `10,11` (3,3 and 4,4).

Each link is then set with either 100Mb or 1Gb connections.

```
11 22
1 1 50
2 2 50
0 0 10
0 0 10
0 0 10
0 0 10
0 0 10
0 0 10
0 0 10
3 3 100
4 4 100
0 2 1000000000.0 0
0 3 1000000000.0 0
1 2 1000000000.0 0
1 3 1000000000.0 0
2 3 100000000.0 0
2 4 1000000000.0 0
2 5 1000000000.0 0
2 6 100000000.0 0
3 4 100000000.0 0
3 5 100000000.0 0
3 6 100000000.0 0
4 7 100000000.0 0
4 8 1000000000.0 0
5 7 1000000000.0 0
5 8 100000000.0 0
6 7 100000000.0 0
6 8 100000000.0 0
7 8 100000000.0 0
7 9 1000000000.0 0
7 10 1000000000.0 0
8 9 1000000000.0 0
8 10 1000000000.0 0
```

Visualized it looks like this, with red and blue indicating multiple VNR allocated paths.

![](doc/images/readme-example.png)


### Virtual Network Request format

First line: <num_node>, <num_edge>, <split_or_not> (not used), <arrive_timestep>, <VNR_duration>, <topology_num> (not used), <max_distance>

Then node lines: <node_loc_x>, <noc_loc_y>, <CPU_requirement>

Then edge lines: <from_node>, <to_node>, <bandwidth_requirement>, <delay_requirement> (not used)

Here is one such VNR using the SN from the above example which has been embedded onto the SN.

```
2 3 0 0 10 0 0
1 1 50
3 3 100
0 1 100000000.0 0
```

### Output file format

First line: the command line used to generate the results.

Then each line: <VNR_id>, <arrive_or_depart_timestep>, <VNR_duration>, <arrive_or_depart>, <accepted_or_not>, <total_revenue>, <total_cost>, <num_CBS_nodes>, <runtime_seconds>.
