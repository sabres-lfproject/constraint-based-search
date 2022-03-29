#include "utility.h"

#include <tuple>
#include <map>
#include <vector>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/copy.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <chrono>
#include <unordered_map>

#define TIMEOUT 60

using namespace boost;
using namespace std;

typedef vector<int> Path;


struct Compare_Path_Usage{
    bool operator()(const pair<int, double>& p1, const pair<int,double>& p2 ) const{
        return p1.second <= p2.second;
    }
};

typedef std::tuple<int, int, vector<int>, int, int, int, int> Conflict;

typedef std::tuple<int, int, int> Constraint;


class CBS_Node{
public:
    int id;
    double cost;
    int num_conflict;
    int parent_id;

    vector<Path> paths;

    map<int, vector<pair<int,int>>> constraint_list;
    struct compare_node{
        bool operator()(const CBS_Node * n1, const CBS_Node * n2 ) const{
            if(n1->cost == n2->cost){
                return n1->num_conflict >= n2->num_conflict;
            }
            return n1->cost >= n2->cost;
        }
    };
    struct compare_node_focal{
        bool operator()(const CBS_Node * n1, const CBS_Node * n2 ) const{

            return n1->num_conflict >= n2->num_conflict;

        }
    };

    boost::heap::fibonacci_heap<CBS_Node*, boost::heap::compare<CBS_Node::compare_node>>::handle_type open_handle;
    boost::heap::fibonacci_heap<CBS_Node*, boost::heap::compare<CBS_Node::compare_node_focal>>::handle_type focal_handle;
};

class A_Node{
public:
    int sn_id;
    A_Node * parent = nullptr;
    float cost;

    // tie-breaking rules.
    int previously_used_by_path = 0;            // 0, this sn has been never used by this vn; 1, this sn has been mapped to this vn in previous paths.
    int times_used_by_other_paths = 0;          // how many times this node has been used by other vns.

//     no tie-breaking
//    struct compare_node{
//        bool operator()(const A_Node * n1, const A_Node * n2 ) const{
//            return n1->cost >= n2->cost;
//        }
//    };

//     tie-breaking:
    struct compare_node{
        bool operator()(const A_Node * n1, const A_Node * n2 ) const{
            if(n1->cost == n2->cost){
                return n1->times_used_by_other_paths >= n2->times_used_by_other_paths;   // prefer the one that has been used fewer times by other vns.
            }
            return n1->cost >= n2->cost;
        }
    };


    A_Node(){};
};

class CBS_Solver{
public:
    int count_num = 0;

    vector<Path> find_solution(SubstrateGraph & SG,VNRequest & aRequest, double w);
    Path find_requirement_path(std::tuple<int,int,double> & requirement, SubstrateGraph & SN,
                               map<int,vector<int>> & metaNodes, map<int,vector<int>> & metaEdges,
                               vector<pair<int,int>> & constraints, vector<Path> & paths, int this_path_id, bool init_path,VNRequest & vnr);
    void make_path(A_Node * goal, Path & path);
    int myFindNodesWithinConstraints(Node &aNode, int reqID, int maxD, SubstrateGraph &SN,
                                                 vector<int> & validNodeIDs);
    void update_VNR_info(vector<Path> paths, VNRequest & vnr,SubstrateGraph & SG);
    double calculate_mapping_cost(vector<Path> paths, VNRequest & vnr,SubstrateGraph & SG,vector<std::tuple<int, int, double>>& requirement_list);
private:
    vector<int> getNeighbors(int nodeID,map<int, vector<int>> metaNodes,
                             map<int,vector<int>> metaEdges,SubstrateGraph & SN);
    Conflict check_conflicts(CBS_Node * n, vector<Path> & paths,VNRequest & aRequest,
                             vector<std::tuple<int,int,double>> & requirement_list, SubstrateGraph & SN);
    int count_num_conflicts(CBS_Node* n, vector<Path>& paths,VNRequest & aRequest,
                            vector<std::tuple<int, int, double>>& requirement_list, SubstrateGraph & SN);
    map<int,int> get_used_sn(vector<Path> & paths, int skip_vn_id, int upto_path_id);
};


