#include "CBS.h"

// helper functions
void print_all_paths(const vector<Path>& paths) {
    for (Path p : paths) {
        for (auto a : p) {
            cout << a << ", ";
        }
        cout << endl;
    }
}

void print_meta_nodes(const map<int, vector<int>>& meta_nodes, int numSNNodes){

    for(auto it = meta_nodes.begin(); it!= meta_nodes.end(); it++){
        cout << it->first << ": ";
        for (unsigned long i =0; i<it->second.size(); i++){
            cout << it->second[i] << " ";
        }
        cout << endl;
    }

}

void print_meta_edges(const map<int,vector<int>> & meta_edges){
    for(auto it = meta_edges.begin(); it!= meta_edges.end(); it++){
        cout << it->first << ": ";
        for (unsigned long i =0; i<it->second.size(); i++){
            cout << it->second[i] << " ";
        }
        cout << endl;
    }
}


void print_requirements(vector<std::tuple<int, int, double>>& requirement_list){
    for(auto t: requirement_list){
        cout << get<0>(t) << ", " << get<1>(t) << ", " << get<2>(t) << "\n";
    }
}

vector<Path> CBS_Solver::find_solution(SubstrateGraph & SG, VNRequest & aRequest, double w){

    auto start = chrono::high_resolution_clock::now();

    SubstrateGraph SG_copy = SG;
    int num_nodes = 0;

    int num_vertices = SG_copy.nodeNum;

    // ====== add VN nodes to SN network  =======.

    map<int, vector<int>> metaNodes;  // list of lists of VN-SN neighbors.
    map<int, vector<int>> metaEdges;


    // make all substrate nodes untouched ( required in findNodesWithinConstraints() )
    for (int i = 0; i < num_vertices; i++) {
        SG_copy.nodes[i].touched = false;
    }

    for (int i = num_vertices; i < aRequest.nodeNum+num_vertices; i++) {

        // This function also adds nodes to meta_nodes.
        int validNodeCount = myFindNodesWithinConstraints(aRequest.nodes[i-num_vertices],
                                                    aRequest.reqID, aRequest.maxD, SG_copy, metaNodes[i]);

        if (validNodeCount == 0) {
            cout << "findNodesWithinConstraints() returned 0" << endl;
            // return empty solution failed.
            return vector<Path>();
        }

        for (int j = 0; j < validNodeCount; j++) {

            // add meta edges to the graph.
            metaEdges[i].push_back(metaNodes[i][j]);
            metaEdges[metaNodes[i][j]].push_back(i);

        }
    }


    /**
     * create a list of requirements based on the VNR.
     * Path id is determined by the order of finding solutions for requirements
     *  The path id is: (req_1, req_2, ..., req_n)
     */

    vector<std::tuple<int,int,double>> requirement_list;

    // create requirements for all edges
    for(int i =0; i<aRequest.edgeNum; i++){

        requirement_list.push_back(make_tuple(aRequest.edges[i].from+num_vertices
                                              ,aRequest.edges[i].to+num_vertices,
                                              aRequest.edges[i].bw));
    }

    // prints: VNR vertex to VNR vertex, bandwidth req.
    print_requirements(requirement_list);

    // CBS storage.
    boost::heap::fibonacci_heap<CBS_Node*, boost::heap::compare<CBS_Node::compare_node_focal>> FOCAL;
    boost::heap::fibonacci_heap<CBS_Node*, boost::heap::compare<CBS_Node::compare_node>> OPEN;
    vector<CBS_Node*> node_track; // for memory release

    CBS_Node* root = new CBS_Node();
    root->id = 0;
    num_nodes++;

    // find init solutions
    int req_count = 0;
    for (auto req: requirement_list){
        Path p = (find_requirement_path(req, SG_copy, metaNodes, metaEdges,
                root->constraint_list[req_count],root->paths,req_count,true,aRequest));
        if (p.empty()){
            cout << "No solution for a requirement." << endl;
            return vector<Path>();
        }
        else{
            root->paths.push_back(p);
            req_count++;
        }
    }

    root->cost = calculate_mapping_cost(root->paths, aRequest, SG_copy,requirement_list);
    root->num_conflict = count_num_conflicts(root,root->paths, aRequest, requirement_list, SG_copy);
    root->open_handle = OPEN.push(root);
    root->focal_handle = FOCAL.push(root);
    node_track.push_back(root);

    while (!FOCAL.empty() ) {

        double f_min = OPEN.top()->cost;
        CBS_Node* curr = FOCAL.top();
        FOCAL.pop();

        OPEN.erase(curr->open_handle);

        num_nodes++;
        if(num_nodes%100 == 0){

            auto p1 = chrono::high_resolution_clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(p1 - start).count()/1000 >= TIMEOUT){
                cout << "CPP Timeout." << endl;
                for (auto node : node_track) {
                    delete node;
                }
                return vector<Path>();
            }

        }

        // check for conflicts
        Conflict conflict = check_conflicts(curr, curr->paths, aRequest, requirement_list, SG_copy);


        if (get<0>(conflict) == -1) {
            cout << "find solution with cost: " << curr->cost << endl;

            for (Path p : curr->paths) {
                for (auto a : p) {
                    cout << a << ", ";
                }
                cout << endl;
            }
            vector<Path> solution = curr->paths;
            cout << "solution cost is: " << calculate_mapping_cost(curr->paths, aRequest, SG_copy,requirement_list) << endl;
            for (auto node : node_track) {
                delete node;
            }


            return solution;
        }

        // create constraints for conflicts
        vector<Constraint> cons_v;
        vector<Constraint> cons_v2;

        if (get<0>(conflict) == 2) {   


            cons_v.push_back(make_tuple(get<1>(conflict), get<3>(conflict), get<4>(conflict)));

            for (auto p_id : get<2>(conflict)) {
                cons_v2.push_back(make_tuple(p_id, get<5>(conflict), get<6>(conflict)));

            }

        }
        else if (get<0>(conflict) == 0) {

            for (auto p_id : get<2>(conflict)) {
                cons_v.push_back(make_tuple(p_id, get<3>(conflict), get<4>(conflict)));
            }
        }
        else if (get<0>(conflict) == 1) {
            for (auto p_id : get<2>(conflict)) {
                cons_v.push_back(make_tuple(p_id, get<3>(conflict), get<4>(conflict)));

            }
        }

        if (cons_v2.empty()) {

            for (Constraint cons : cons_v) {
                int path_to_update = get<0>(cons);
                CBS_Node* child = new CBS_Node();
                child->id = curr->id + 1;
                child->paths = curr->paths;
                child->parent_id = curr->id;
                child->constraint_list = curr->constraint_list;
                if(find(child->constraint_list[path_to_update].begin(),child->constraint_list[path_to_update].end(),make_pair(get<1>(cons), get<2>(cons))) == child->constraint_list[path_to_update].end()){
                    child->constraint_list[path_to_update].push_back(make_pair(get<1>(cons), get<2>(cons)));
                }
                else{
                    delete child;
                    continue;
                }
                Path new_path = find_requirement_path(requirement_list[path_to_update],
                                                                       SG_copy,metaNodes, metaEdges,
                                                                       child->constraint_list[path_to_update], child->paths, path_to_update,false, aRequest);

                if (new_path.size() > 0) {
                    child->paths[path_to_update] = new_path;

                    child->cost = calculate_mapping_cost(child->paths, aRequest, SG_copy,requirement_list);
                    child->num_conflict = count_num_conflicts(child, child->paths, aRequest, requirement_list, SG_copy);

                    child->open_handle = OPEN.push(child);

                    if (child->cost <= w*f_min){
                        child->focal_handle = FOCAL.push(child);
                    }

                    node_track.push_back(child);
                    count_num++;
                }
                else {
                    delete child;
                }
            }
        }
        else { 
        
            CBS_Node* child = new CBS_Node();
            child->id = curr->id + 1;
            child->paths = curr->paths;
            child->constraint_list = curr->constraint_list;
            child->parent_id = curr->id;
            bool generate_child_node = false;
            for (Constraint cons : cons_v2) {
                int path_to_update = get<0>(cons);
                if(find(child->constraint_list[path_to_update].begin(),child->constraint_list[path_to_update].end(),make_pair(get<1>(cons), get<2>(cons))) == child->constraint_list[path_to_update].end()){
                    child->constraint_list[path_to_update].push_back(make_pair(get<1>(cons), get<2>(cons)));
                    generate_child_node = true;
                }
                else{
                    continue;
                }

                Path new_path = find_requirement_path(requirement_list[path_to_update],
                                                      SG_copy,metaNodes, metaEdges,
                                                                       child->constraint_list[path_to_update],child->paths, path_to_update,false, aRequest);
                if (new_path.size() > 0) {
                    child->paths[path_to_update] = new_path;
                }
                else {
                    generate_child_node = false;
                    //cout << "can't find some path" << endl;
                    break;
                }
            }
            if (generate_child_node) {
                child->cost = calculate_mapping_cost(child->paths, aRequest, SG_copy,requirement_list);
                child->num_conflict = count_num_conflicts(child, child->paths, aRequest, requirement_list, SG_copy);

                child->open_handle = OPEN.push(child);
                if (child->cost <= w*f_min){
                    child->focal_handle =FOCAL.push(child);
                }
                node_track.push_back(child);
                count_num++;
            }
            else {
                delete child;
            }

            for (Constraint cons : cons_v) {
                int path_to_update = get<0>(cons);
                CBS_Node* child = new CBS_Node();
                child->id = curr->id + 1;
                child->paths = curr->paths;
                child->constraint_list = curr->constraint_list;
                child->parent_id = curr->id;
                if(find(child->constraint_list[path_to_update].begin(),child->constraint_list[path_to_update].end(),make_pair(get<1>(cons), get<2>(cons))) == child->constraint_list[path_to_update].end()){
                    child->constraint_list[path_to_update].push_back(make_pair(get<1>(cons), get<2>(cons)));
                }
                else{

                    continue;
                }
                Path new_path = find_requirement_path(requirement_list[path_to_update],
                                                      SG_copy,metaNodes, metaEdges,
                                                      child->constraint_list[path_to_update],child->paths, path_to_update,false, aRequest);

                if (new_path.size() > 0) {
                    child->paths[path_to_update] = new_path;

                    child->cost = calculate_mapping_cost(child->paths, aRequest, SG_copy,requirement_list);
                    child->num_conflict = count_num_conflicts(child, child->paths, aRequest, requirement_list, SG_copy);

                    child->open_handle = OPEN.push(child);
                    if (child->cost <= w*f_min){
                        child->focal_handle =FOCAL.push(child);
                    }
                    node_track.push_back(child);
                    count_num++;
                }
                else {
                    delete child;
                }
            }
        }
        if (!OPEN.empty() && f_min < OPEN.top()->cost){
            double old_b = w*f_min;
            double new_b = w*OPEN.top()->cost;
            for(auto node: OPEN){
                if(node->cost > old_b && node->cost <= new_b){
                    node->focal_handle =FOCAL.push(node);
                }
            }
        }
    }
    for (auto node : node_track) {
        delete node;
    }

    cout << "No solution CBS." << endl;

    return vector<Path>();

}

inline int get_sn_mapped_to_vn_in_other_paths(int vn, vector<Path> & paths, int up_to_this_path_id){

    for (int i =0; i<up_to_this_path_id; i++){
        if(paths[i][0]==vn){
            return paths[i][1];
        }
        if(paths[i].size()<3){
            continue;
        }
        if(paths[i][paths[i].size()-1] == vn){
            return paths[i][paths[i].size()-2];
        }
    }
    return -1;
}


Path CBS_Solver::find_requirement_path(std::tuple<int,int,double> & requirement, SubstrateGraph & SN,
                                       map<int,vector<int>> & metaNodes, map<int,vector<int>> & metaEdges,
                                       vector<pair<int,int>> & constraints, vector<Path> & paths, int this_path_id, bool init_path, VNRequest & vnr) {


    boost::heap::fibonacci_heap<A_Node *, boost::heap::compare<A_Node::compare_node>> OPEN;
    vector<A_Node*> node_track;
    vector<int> CLOSED; // closed list to avoid loops. map (sn_node_id, cost)
    Path path;

    A_Node * root = new A_Node();

    int start_vn = get<0>(requirement);
    int goal_vn = get<1>(requirement);

    root->sn_id = start_vn;
    root->cost = 0;

    OPEN.push(root);


    // record how many times sub nodes have  been used by other vn nodes. The vn in this pathfinding is not counted.
    map<int, int> sn_usage_counter_start = get_used_sn(paths, start_vn, paths.size());

    // Get the previously mapped sn node. Potentially fewer conflicts.
    int sn_mapped_to_start_before = get_sn_mapped_to_vn_in_other_paths(start_vn, paths, this_path_id);

    map<int, int> sn_usage_counter_goal = get_used_sn(paths, goal_vn, paths.size());
    int sn_mapped_to_goal_before = get_sn_mapped_to_vn_in_other_paths(goal_vn, paths, this_path_id);

    while(!OPEN.empty()){
        auto top = OPEN.top();
        OPEN.pop();

        if(top->sn_id == goal_vn ){
            if(top->parent->parent->sn_id==start_vn){
                continue;
            }
            else{
                make_path(top, path);

                reverse(path.begin(), path.end());

                for (auto node : node_track) {
                    delete node;
                }

                return path;
            }

        }


        // get neighbors
        vector<int> neighbors = getNeighbors(top->sn_id, metaNodes,metaEdges,SN);

        for(int n: neighbors){

            if(top->sn_id < SN.nodeNum && n>= SN.nodeNum && n!=goal_vn){
                continue; // Skip. Don't go through VN. (No SN-VN-SN)
            }
            if (top->sn_id < SN.nodeNum && n < SN.nodeNum) {
                //Get the Bandwidth resource from SN graph.
                double SN_BW = SN.edges[SN.edgeMap[make_pair(top->sn_id, n)]].rest_bw;
                double want_BW = get<2>(requirement);

                if (want_BW > SN_BW) {
                    continue; // Not enough resource skip.
                }
            }

            auto iter = find(constraints.begin(),constraints.end(), make_pair(top->sn_id, n));
            auto iter2 = find(constraints.begin(),constraints.end(), make_pair(n, top->sn_id));
            auto visited = find(CLOSED.begin(), CLOSED.end(), n);
            if(iter==constraints.end() && iter2 == constraints.end() && visited == CLOSED.end()){

                A_Node * child = new A_Node();
                child->sn_id = n;

                if(top->sn_id==start_vn){ // cost for vn-sn mapping.
                    if (!init_path){
                        if(n==sn_mapped_to_start_before){
                            child->previously_used_by_path = 1;
                        }

                        child->times_used_by_other_paths = sn_usage_counter_start[n];
                    }


                    child->cost = top->cost + 1;

                }else{  // cost for sn-sn and sn-vn mapping.

//                    float h = 0;
                    vector<int> n_neighbors = getNeighbors(n, metaNodes,metaEdges,SN);
                    bool connected_to_goal = (find(n_neighbors.begin(),n_neighbors.end(), goal_vn) != n_neighbors.end());
                    if(connected_to_goal && (n==sn_mapped_to_goal_before) & (!init_path)){
                        child->previously_used_by_path = 1;
                        child->times_used_by_other_paths = sn_usage_counter_goal[n];
                    }
                    child->cost = top->cost + 1;

                }


                child->parent = top;
                OPEN.push(child);
                node_track.push_back(child);

            }
        }


        CLOSED.push_back(top->sn_id);
    }
    return path;
}

int CBS_Solver::myFindNodesWithinConstraints(Node &aNode, int reqID, int maxD, SubstrateGraph &SN,
                                               vector<int> & validNodeIDs) {

    int i, count = 0;
    validNodeIDs.clear();

    for (i = 0; i < SN.nodeNum; i++) {
        if ((SN.nodes[i].distanceFrom(aNode) <= maxD) &&
            (SN.nodes[i].rest_cpu >= aNode.cpu) && (SN.nodes[i].touched == false)) {
            validNodeIDs.push_back(i);
            count++;
        }
    }

    return count;
}

vector<int> CBS_Solver::getNeighbors(int nodeID,map<int, vector<int>> metaNodes,
                                     map<int,vector<int>> metaEdges,SubstrateGraph & SN){
    vector<int> neighbors;

    // only VN->SN
    if(nodeID >= SN.nodeNum){
        return metaEdges[nodeID];
    }
    else{ // SN->SN, VN->VN
        neighbors.insert(neighbors.end(), SN.neighbor[nodeID].begin(), SN.neighbor[nodeID].end());
        neighbors.insert(neighbors.end(), metaEdges[nodeID].begin(), metaEdges[nodeID].end());
    }

    return neighbors;
}

void CBS_Solver::make_path(A_Node * goal, Path & path){

    A_Node * curr = goal;
    while(curr != nullptr){
        path.push_back(curr->sn_id);
        curr = curr->parent;
    }
}

Conflict CBS_Solver::check_conflicts(CBS_Node * n, vector<Path> & paths, VNRequest & aRequest,
                                     vector<std::tuple<int,int,double>> & requirement_list, SubstrateGraph & SN){
    int path_id = 0;
    int num_SN_nodes = SN.nodeNum;

    map<int, double> used_CPU_list;
    map<int, vector<int>> paths_used_node;
    map<int, int> VN_SN_one_to_one;
    map<int, int> SN_VN_one_to_one;
    map<pair<int,int>,double> used_BD_list;
    map < pair<int, int>, vector<int>> paths_used_edge;

    for (Path p: paths){


        auto it = VN_SN_one_to_one.find(p[0]);
        if (it == VN_SN_one_to_one.end()) { // this vn hasn't been mapped yet.

            // check if other vns already took this sn.
            auto it_sn_vn = SN_VN_one_to_one.find(p[1]);
            if(it_sn_vn!=SN_VN_one_to_one.end()){ // another vn is using this sn.

                Conflict c = make_tuple(2, path_id, paths_used_node[p[1]], p[0], p[1], SN_VN_one_to_one[p[1]],p[1]);
                return c;
            }
            else{ // if no one is using it,
                VN_SN_one_to_one[p[0]] = p[1];
                SN_VN_one_to_one[p[1]] = p[0];
                used_CPU_list[p[1]] += aRequest.nodes[p[0]-num_SN_nodes].cpu;
                paths_used_node[p[1]].push_back(path_id);
            }
        }
        else { // this vn has been mapped.
            if (p[1] != VN_SN_one_to_one[p[0]]) {
                Conflict c = make_tuple(2, path_id, paths_used_node[VN_SN_one_to_one[p[0]]], p[0], p[1], p[0],VN_SN_one_to_one[p[0]]);
                return c;
            }
            else{ 
                paths_used_node[p[1]].push_back(path_id);
            }
        }


        if (used_CPU_list[p[1]] > SN.nodes[p[1]].rest_cpu) {
            paths_used_node[p[1]].push_back(path_id);
            Conflict c = make_tuple(0, -1, paths_used_node[p[1]], p[0], p[1], -1,-1);
            return c;
        }

        if (p.size() == 2) {
            path_id++;
            continue;
        }

        int end_idx = p.size() - 1;
        int last_sn_idx = end_idx - 1;


        auto it2 = VN_SN_one_to_one.find(p[end_idx]);
        if (it2 == VN_SN_one_to_one.end()) {

            auto it_sn_vn2 = SN_VN_one_to_one.find(p[last_sn_idx]);
            if(it_sn_vn2!=SN_VN_one_to_one.end()){

                Conflict c = make_tuple(2, path_id, paths_used_node[p[last_sn_idx]], p[end_idx], p[last_sn_idx], SN_VN_one_to_one[p[last_sn_idx]],p[last_sn_idx]);
                return c;
            }
            else{
                VN_SN_one_to_one[p[end_idx]] = p[last_sn_idx];
                SN_VN_one_to_one[p[last_sn_idx]] = p[end_idx];
                used_CPU_list[p[last_sn_idx]] += aRequest.nodes[p[end_idx]-num_SN_nodes].cpu;
                paths_used_node[p[last_sn_idx]].push_back(path_id);
            }

        }
        else {
            if (p[last_sn_idx] != VN_SN_one_to_one[p[end_idx]]) {
                Conflict c = make_tuple(2, path_id, paths_used_node[VN_SN_one_to_one[p[end_idx]]], p[end_idx], p[last_sn_idx], p[end_idx], VN_SN_one_to_one[p[end_idx]]);
                return c;
            }
            else{
                paths_used_node[p[last_sn_idx]].push_back(path_id);
            }
        }


        if (used_CPU_list[p[last_sn_idx]] > SN.nodes[p[last_sn_idx]].rest_cpu) {
            paths_used_node[p[last_sn_idx]].push_back(path_id);
            Conflict c = make_tuple(0, -1, paths_used_node[p[last_sn_idx]], p[end_idx], p[last_sn_idx], -1, -1);
            return c;
        }

        for (unsigned long p_i = 1; p_i < p.size() - 2; p_i++) {

            used_BD_list[make_pair(p[p_i], p[p_i + 1])] += get<2>(requirement_list[path_id]);
            used_BD_list[make_pair(p[p_i + 1], p[p_i])] += get<2>(requirement_list[path_id]);

            paths_used_edge[make_pair(p[p_i], p[p_i + 1])].push_back(path_id);
            paths_used_edge[make_pair(p[p_i + 1], p[p_i])].push_back(path_id);
            if (used_BD_list[make_pair(p[p_i], p[p_i + 1])] > SN.edges[SN.edgeMap[make_pair(p[p_i],p[p_i + 1])]].rest_bw) {

                Conflict c = make_tuple(1, -1, paths_used_edge[make_pair(p[p_i], p[p_i + 1])], p[p_i], p[p_i + 1], -1,-1);
                return c;
            }
        }
        path_id++;
    }
    vector<int> empty;
    return make_tuple (-1,-1, empty,-1,-1,-1, -1); // no conflict -> solution
}

int CBS_Solver::count_num_conflicts(CBS_Node* n, vector<Path>& paths, VNRequest & aRequest,
                                    vector<std::tuple<int, int, double>>& requirement_list, SubstrateGraph & SN) {
    int num_conflicts = 0;
    int path_id = 0;
    int num_SN_nodes = SN.nodeNum;

    map<int, double> used_CPU_list;
    map<int, vector<int>> paths_used_node;
    map<int, int> VN_SN_one_to_one; // enforce the one-to-one mapping of vN and SN. [vn] -> sn.
    map<int, int> SN_VN_one_to_one; // a reverse lookup for SN-VN.
    map<pair<int,int>,double> used_BD_list;
    map < pair<int, int>, vector<int>> paths_used_edge;

    for (Path p: paths){

        auto it = VN_SN_one_to_one.find(p[0]);
        if (it == VN_SN_one_to_one.end()) { // this vn hasn't been mapped yet.

            auto it_sn_vn = SN_VN_one_to_one.find(p[1]);
            if(it_sn_vn!=SN_VN_one_to_one.end()){ // another vn is using this sn.

                num_conflicts++;
            }
            else{ // if no one is using it,
                VN_SN_one_to_one[p[0]] = p[1];
                SN_VN_one_to_one[p[1]] = p[0];
                used_CPU_list[p[1]] += aRequest.nodes[p[0]-num_SN_nodes].cpu;
                paths_used_node[p[1]].push_back(path_id);
            }
        }
        else {
            if (p[1] != VN_SN_one_to_one[p[0]]) {
                num_conflicts++;
            }
            else{ 
                paths_used_node[p[1]].push_back(path_id);
            }
        }

        if (used_CPU_list[p[1]] > SN.nodes[p[1]].rest_cpu) {

            num_conflicts++;
        }

        if (p.size() == 2) {
            path_id++;
            continue;
        }

        int end_idx = p.size() - 1;
        int last_sn_idx = end_idx - 1;

        auto it2 = VN_SN_one_to_one.find(p[end_idx]);
        if (it2 == VN_SN_one_to_one.end()) {
            auto it_sn_vn2 = SN_VN_one_to_one.find(p[last_sn_idx]);
            if(it_sn_vn2!=SN_VN_one_to_one.end()){ // another vn is using this sn.
                num_conflicts++;
            }
            else{
                VN_SN_one_to_one[p[end_idx]] = p[last_sn_idx];
                SN_VN_one_to_one[p[last_sn_idx]] = p[end_idx];
                used_CPU_list[p[last_sn_idx]] += aRequest.nodes[p[end_idx]-num_SN_nodes].cpu;
                paths_used_node[p[last_sn_idx]].push_back(path_id);
            }

        }
        else {
            if (p[last_sn_idx] != VN_SN_one_to_one[p[end_idx]]) {
                num_conflicts++;
            }
            else{
                paths_used_node[p[last_sn_idx]].push_back(path_id);
            }
        }

        if (used_CPU_list[p[last_sn_idx]] > SN.nodes[p[last_sn_idx]].rest_cpu) {
            num_conflicts++;
        }

        for (unsigned long p_i = 1; p_i < p.size() - 2; p_i++) {
            used_BD_list[make_pair(p[p_i], p[p_i + 1])] += get<2>(requirement_list[path_id]);
            used_BD_list[make_pair(p[p_i + 1], p[p_i])] += get<2>(requirement_list[path_id]);

            paths_used_edge[make_pair(p[p_i], p[p_i + 1])].push_back(path_id);
            paths_used_edge[make_pair(p[p_i + 1], p[p_i])].push_back(path_id);
            if (used_BD_list[make_pair(p[p_i], p[p_i + 1])] > SN.edges[SN.edgeMap[make_pair(p[p_i],p[p_i + 1])]].rest_bw) {
                num_conflicts++;
            }
        }
        path_id++;
    }
    return num_conflicts;
}

double CBS_Solver::calculate_mapping_cost(vector<Path> paths, VNRequest & vnr,SubstrateGraph & SG,
                                        vector<std::tuple<int, int, double>>& requirement_list){
    double cost_sum = 0.0;
    int num_SN_nodes = SG.nodeNum;
    int path_id = 0;
    map<int,int> VN_SN;

    for(Path p: paths){
        auto it = VN_SN.find(p[0]);
        if (it == VN_SN.end()){
            cost_sum += vnr.nodes[p[0]-num_SN_nodes].cpu;
            VN_SN[p[0]] = p[1];
        }

        int end_idx = p.size() - 1;
        int last_sn_idx = end_idx - 1;
        auto it2 = VN_SN.find(p[end_idx]);
        if (it2 == VN_SN.end()){
            cost_sum += vnr.nodes[p[end_idx]-num_SN_nodes].cpu;
            VN_SN[p[end_idx]] = p[last_sn_idx];
        }

        for (unsigned long p_i = 1; p_i<p.size()-2; p_i++){
            cost_sum += get<2>(requirement_list[path_id]);
        }
        path_id++;
    }
    return cost_sum;
}

void CBS_Solver::update_VNR_info(vector<Path> paths, VNRequest & vnr,SubstrateGraph & SG){
    int vnr_edge_id = 0;
    for(Path p: paths){
        vnr.nodes[p[0]-SG.nodeNum].substrateID = SG.substrateID;
        vnr.nodes[p[0]-SG.nodeNum].subNodeID = p[1];
        SG.nodes[vnr.nodes[p[0]-SG.nodeNum].subNodeID].touched = true;

        int last_index = p.size()-1;
        int second_last = last_index -1;
        vnr.nodes[p[last_index]-SG.nodeNum].substrateID = SG.substrateID;
        vnr.nodes[p[last_index]-SG.nodeNum].subNodeID = p[second_last];
        SG.nodes[vnr.nodes[p[last_index]-SG.nodeNum].subNodeID].touched = true;

        vnr.edges[vnr_edge_id].substrateID = SG.substrateID;

        if(p.size()>3){
            for(unsigned long j =1; j<p.size()-2;j++){
                int from = p[j];
                int to = p[j+1];

                vnr.edges[vnr_edge_id].pathLen++;
                int eID = SG.edgeMap[make_pair(from,to)];

                vnr.edges[vnr_edge_id].subPath.push_back(eID);
                vnr.edges[vnr_edge_id].subBW.push_back(vnr.edges[vnr_edge_id].bw);

            }
        }

        vnr_edge_id++;
    }
}

map<int,int> CBS_Solver::get_used_sn(vector<Path> & paths, int skip_vn_id, int upto_path_id){
    map<int,int> used_sn;

    //record how many times a sn get used by vn.
    int path_counter = 0;
    for(Path p: paths){
        if(path_counter>=upto_path_id){
            return used_sn;
        }
        if(p[0] != skip_vn_id){
            used_sn[p[1]]++;
        }
        if(p.size()>3 and p[p.size()-1] != skip_vn_id){
            used_sn[p[p.size()-2]]++;
        }
        path_counter++;
    }

    return used_sn;
}
