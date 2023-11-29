/*
 * Utility.cpp
 *
 *  Created on: Jul 13, 2008
 *      Author: nmmkchow
 */

#include "utility.hpp"

#include "def.hpp"

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous Functions                                                   //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

void saveMapping(VNRequest &VNR, SubstrateGraph &SG, string save_file_path) {
  int i, j;
  FILE *saveto = fopen(save_file_path.c_str(), "w");

  fprintf(saveto, "{\n");

  fprintf(saveto, "\t\"nodes\": [\n");
  for (i = 0; i < VNR.nodeNum; i++) {
    fprintf(saveto, "\t\t{\"node\": %d, \"cpu\": %f, \"leftover\": %f}", 
                    VNR.nodes[i].subNodeID,
                    VNR.nodes[i].cpu, 
                    SG.nodes[VNR.nodes[i].subNodeID].rest_cpu);

    // only add comma when we have more to add
    if ((i + 1) < VNR.nodeNum) {
      fprintf(saveto, ",");
    }

    fprintf(saveto, "\n");
  }
  // always assume edges
  fprintf(saveto, "\t],\n");

  fprintf(saveto, "\t\"edges\": [\n");
  for (i = 0; i < VNR.edgeNum; i++) {
    for (j = 0; j < VNR.edges[i].pathLen; j++) {
      fprintf(saveto, "\t\t{\"src\": %d, \"dst\": %d, \"cost\": %f}",
             SG.edges[VNR.edges[i].subPath[j]].from,
             SG.edges[VNR.edges[i].subPath[j]].to,
             VNR.edges[i].subBW[j]);
      if ((j + 1) < VNR.edges[i].pathLen) {
         fprintf(saveto, ",");
      }

      fprintf(saveto, "\n");
    }
  }

  fprintf(saveto, "\t]\n");
  fprintf(saveto, "}\n");

  fclose(saveto);
}

void printMapping(VNRequest &VNR, SubstrateGraph &SG) {
  int i, j;

  // considering only one InP

  cout << "Node Mapping" << endl;
  for (i = 0; i < VNR.nodeNum; i++) {
    printf("VNode: %2d @(%2d, %2d) CPU = %.4lf <--> PNode: %2d @(%2d, %2d) RCPU = %.4f \n",
           i,
           VNR.nodes[i].x,
           VNR.nodes[i].y,
           VNR.nodes[i].cpu,
           /*VNR.nodes[i].substrateID,*/ VNR.nodes[i].subNodeID,
           SG.nodes[VNR.nodes[i].subNodeID].x,
           SG.nodes[VNR.nodes[i].subNodeID].y,
           SG.nodes[VNR.nodes[i].subNodeID].rest_cpu);
  }

  cout << "Edge Mapping" << endl;
  for (i = 0; i < VNR.edgeNum; i++) {
    printf("VEdge: %2d @(%2d, %2d) BW = %.4lf <--> PEdges:", i, VNR.edges[i].from, VNR.edges[i].to, VNR.edges[i].bw);
    for (j = 0; j < VNR.edges[i].pathLen; j++) {
      printf(" %3d[%.4lf, %.4f] @(%2d, %2d)",
             VNR.edges[i].subPath[j],
             VNR.edges[i].subBW[j],
             SG.edges[VNR.edges[i].subPath[j]].bw,
             SG.edges[VNR.edges[i].subPath[j]].from,
             SG.edges[VNR.edges[i].subPath[j]].to);
    }
    printf("\n");
  }
}

void getDifferentStress(SubstrateGraph &SG,
                        double &mNS,
                        double &aNS,
                        double &mLS,
                        double &aLS,
                        double &sdNS,
                        double &sdLS) {
  int i;
  double tStress;
  mNS = mLS = aNS = aLS = sdNS = sdLS = 0;

  for (i = 0; i < SG.nodeNum; i++) {
    tStress = (SG.nodes[i].cpu - SG.nodes[i].rest_cpu) / SG.nodes[i].cpu;
    if (tStress > mNS) {
      mNS = tStress;
    }
    aNS += tStress;
  }
  aNS /= SG.nodeNum;

  for (i = 0; i < SG.nodeNum; i++) {
    tStress = (SG.nodes[i].cpu - SG.nodes[i].rest_cpu) / SG.nodes[i].cpu;
    sdNS += ((aNS - tStress) * (aNS - tStress));
  }
  sdNS = sqrt((sdNS / SG.nodeNum));

  for (i = 0; i < SG.edgeNum; i++) {
    tStress = (SG.edges[i].bw - SG.edges[i].rest_bw) / SG.edges[i].bw;
    if (tStress > mLS) {
      mLS = tStress;
    }
    aLS += tStress;
  }
  aLS /= SG.edgeNum;

  for (i = 0; i < SG.edgeNum; i++) {
    tStress = (SG.edges[i].bw - SG.edges[i].rest_bw) / SG.edges[i].bw;
    sdLS += ((aLS - tStress) * (aLS - tStress));
  }
  sdLS = sqrt((sdLS / SG.edgeNum));
}

double getRevenue(VNRequest &aRequest, double __MULT, double &nodeRev, double &edgeRev) {
  int i;
  nodeRev = edgeRev = 0;

  for (i = 0; i < aRequest.nodeNum; i++) {
    nodeRev += aRequest.nodes[i].cpu;
  }

  for (i = 0; i < aRequest.edgeNum; i++) {
    edgeRev += aRequest.edges[i].bw;
  }

  return nodeRev + __MULT * edgeRev;
}

double
getCost(VNRequest &VNR, SubstrateGraph &SG, double __MULT, double &nodeCost, double &edgeCost, bool aOne, bool bOne) {
  int i, j;
  double temp;
  nodeCost = edgeCost = 0;

  for (i = 0; i < VNR.nodeNum; i++) {
    /*if (bOne) {
      temp = 1.0 / (SG.nodes[VNR.nodes[i].subNodeID].rest_cpu + EPSILON)
        * VNR.nodes[i].cpu;
    }
    else {*/
    temp = VNR.nodes[i].cpu;
    //}
    nodeCost += temp;
  }

  for (i = 0; i < VNR.edgeNum; i++) {
    temp = 0;
    for (j = 0; j < VNR.edges[i].pathLen; j++) {
      /*if (aOne) {
        temp += (1.0 / (SG.edges[VNR.edges[i].subPath[j]].rest_bw + EPSILON)
          * VNR.edges[i].subBW[j]);
      }
      else {*/
      temp += VNR.edges[i].subBW[j];
      //}
    }
    edgeCost += temp;
  }

  return nodeCost + __MULT * edgeCost;
}

void randomPermutation(vector<int> &series) {
  int j;
  int sLen = series.size();
  vector<int> temp(sLen, 0), ret;

  while (sLen) {
    j = rand() % sLen;
    ret.push_back(series[j]);
    series[j] = series[--sLen];
  }

  ret.resize(series.size());
  copy(ret.begin(), ret.end(), series.begin());
}

///////////////////////////////////////////////////////////////////////////////
// Class      : Node                                                         //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

Node::Node(int _x, int _y, double _cpu) : x(_x), y(_y), cpu(_cpu) {
  if (DEBUG) {
    cout << "Node" << endl;
  }
}

#ifdef MYCPP
Node::Node(const Node &o) : x(o.x), y(o.y), cpu(o.cpu) {
  if (DEBUG) {
    cout << "Node Copy" << endl;
  }
}

Node::~Node() {
  if (DEBUG) {
    cout << "~Node" << endl;
  }
}

const Node &Node::operator=(const Node &o) {
  if (DEBUG) {
    cout << "Node=" << endl;
  }
  if (this != &o) {
    x = o.x;
    y = o.y;
    cpu = o.cpu;
  }

  return *this;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Class      : Edge                                                         //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

Edge::Edge(int _from, int _to, double _bw, double _dlay) : from(_from), to(_to), bw(_bw), dlay(_dlay) {
  if (DEBUG) {
    cout << "Edge" << endl;
  }
}

#ifdef MYCPP
Edge::Edge(const Edge &o) : from(o.from), to(o.to), bw(o.bw), dlay(o.dlay) {
  if (DEBUG) {
    cout << "Edge Copy" << endl;
  }
}

Edge::~Edge() {
  if (DEBUG) {
    cout << "~Edge" << endl;
  }
}

const Edge &Edge::operator=(const Edge &o) {
  if (DEBUG) {
    cout << "Edge=" << endl;
  }
  if (this != &o) {
    from = o.from;
    to = o.to;
    bw = o.bw;
    dlay = o.dlay;
  }

  return *this;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Class      : SubstrateGraph                                               //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

SubstrateGraph::SubstrateGraph(string _fileName, int _substrateID) : fileName(_fileName), substrateID(_substrateID) {
  if (DEBUG) {
    cout << "SubGraph" << endl;
  }

  nodeNum = 0;
  edgeNum = 0;

  nodes.clear();
  edges.clear();

  edgeMap.clear();

  initGraph();
}

#ifdef MYCPP
SubstrateGraph::SubstrateGraph(const SubstrateGraph &o) {
  if (DEBUG) {
    cout << "SubGraph Copy" << endl;
  }

  nodeNum = o.nodeNum;
  edgeNum = o.edgeNum;
  substrateID = o.substrateID;
  fileName = o.fileName;

  // copy(o.nodes.begin(), o.nodes.end(), nodes.begin());
  for (int i = 0; i < o.nodeNum; i++) nodes.push_back(o.nodes[i]);
  // copy(o.edges.begin(), o.edges.end(), edges.begin());
  for (int i = 0; i < edgeNum; i++) edges.push_back(o.edges[i]);
}

SubstrateGraph::~SubstrateGraph() {
  if (DEBUG) {
    cout << "~SubGraph" << endl;
  }

  nodes.clear();
  edges.clear();
}

const SubstrateGraph &SubstrateGraph::operator=(const SubstrateGraph &o) {
  if (DEBUG) {
    cout << "SubGraph=" << endl;
  }

  if (this != &o) {
    nodeNum = o.nodeNum;
    edgeNum = o.edgeNum;
    substrateID = o.substrateID;
    fileName = o.fileName;

    // copy(o.nodes.begin(), o.nodes.end(), nodes.begin());
    for (int i = 0; i < o.nodeNum; i++) nodes.push_back(o.nodes[i]);
    // copy(o.edges.begin(), o.edges.end(), edges.begin());
    for (int i = 0; i < edgeNum; i++) edges.push_back(o.edges[i]);
  }

  return *this;
}
#endif

void SubstrateGraph::printNodeStatus() {
  int i;

  cout << "Substrate Graph: " << substrateID << " Node Status" << endl;
  for (i = 0; i < nodeNum; i++) {
    printf("(%2d, %6.3lf)", nodes[i].count, nodes[i].rest_cpu);
  }
  cout << endl;
}

void SubstrateGraph::printEdgeStatus() {
  int i;

  cout << "Substrate Graph: " << substrateID << " Edge Status" << endl;
  for (i = 0; i < edgeNum; i++) {
    printf("(%2d, %6.3lf)", edges[i].count, edges[i].rest_bw);
  }
  cout << endl;
}

void SubstrateGraph::addVNMapping(VNRequest &aRequest) {
  int i, j;

  // add to the substrate network nodes
  for (i = 0; i < aRequest.nodeNum; i++) {
    if (aRequest.nodes[i].substrateID == substrateID) {
      //      cout << "Before adding rest_cpu to node: " << aRequest.nodes[i].subNodeID << " value: " <<
      //      nodes[aRequest.nodes[i].subNodeID].rest_cpu << endl; cout << "Full CPU: " <<
      //      nodes[aRequest.nodes[i].subNodeID].cpu << endl;
      nodes[aRequest.nodes[i].subNodeID].rest_cpu -= aRequest.nodes[i].cpu;
      //      cout << "After adding rest_cpu to node: " << aRequest.nodes[i].subNodeID << " value: " <<
      //      nodes[aRequest.nodes[i].subNodeID].rest_cpu << endl;
      // assert(fabs(nodes[aRequest.nodes[i].subNodeID].rest_cpu + EPSILON) >= fabs(0));
      nodes[aRequest.nodes[i].subNodeID].count++;

      // save request information
      nodes[aRequest.nodes[i].subNodeID].req_ids.push_back(aRequest.reqID);
      nodes[aRequest.nodes[i].subNodeID].node_ids.push_back(i);
      nodes[aRequest.nodes[i].subNodeID].used_cpu.push_back(aRequest.nodes[i].cpu);
    }
  }

  // add to the substrate network links
  for (i = 0; i < aRequest.edgeNum; i++) {
    for (j = 0; j < aRequest.edges[i].pathLen; j++) {
      if (aRequest.edges[i].substrateID == substrateID) {
        //        cout << "Before adding rest_bw to edge: " << aRequest.edges[i].subPath[j] << " value: " <<
        //        edges[aRequest.edges[i].subPath[j]].rest_bw << endl; cout << "Full bandwidth: " <<
        //        edges[aRequest.edges[i].subPath[j]].bw << endl;
        edges[aRequest.edges[i].subPath[j]].rest_bw -= aRequest.edges[i].subBW[j];
        // assert(fabs(edges[aRequest.edges[i].subPath[j]].rest_bw + EPSILON) >= fabs(0));
        edges[aRequest.edges[i].subPath[j]].count++;
        //        cout << "After adding rest_bw to edge: " << aRequest.edges[i].subPath[j] << " value: " <<
        //        edges[aRequest.edges[i].subPath[j]].rest_bw << endl;
        // save request information
        edges[aRequest.edges[i].subPath[j]].req_ids.push_back(aRequest.reqID);
        edges[aRequest.edges[i].subPath[j]].edge_ids.push_back(i);
        edges[aRequest.edges[i].subPath[j]].used_bw.push_back(aRequest.edges[i].subBW[j]);
      }
    }
  }
}

void SubstrateGraph::removeVNMapping(const VNRequest &aRequest) {
  int i, j;
  vector<int>::iterator iter1, iter2;
  vector<double>::iterator iter3;

  // remove from the substrate network nodes
  for (i = 0; i < aRequest.nodeNum; i++) {
    if (aRequest.nodes[i].substrateID == substrateID) {
      nodes[aRequest.nodes[i].subNodeID].rest_cpu += aRequest.nodes[i].cpu;
      // assert(nodes[aRequest.nodes[i].subNodeID].rest_cpu <= nodes[aRequest.nodes[i].subNodeID].cpu + EPSILON);
      nodes[aRequest.nodes[i].subNodeID].count--;

      // remove request information
      iter1 = nodes[aRequest.nodes[i].subNodeID].req_ids.begin();
      iter2 = nodes[aRequest.nodes[i].subNodeID].node_ids.begin();
      iter3 = nodes[aRequest.nodes[i].subNodeID].used_cpu.begin();
      while (iter1 != nodes[aRequest.nodes[i].subNodeID].req_ids.end()) {
        if (*iter1 == aRequest.reqID) {
          nodes[aRequest.nodes[i].subNodeID].req_ids.erase(iter1);
          nodes[aRequest.nodes[i].subNodeID].node_ids.erase(iter2);
          nodes[aRequest.nodes[i].subNodeID].used_cpu.erase(iter3);

          // can break, since only one VNode can map to a PNode
          break;
        }
        iter1++;
        iter2++;
        iter3++;
      }
    }
  }

  // remove from the substrate network links
  for (i = 0; i < aRequest.edgeNum; i++) {
    for (j = 0; j < aRequest.edges[i].pathLen; j++) {
      if (aRequest.edges[i].substrateID == substrateID) {
        edges[aRequest.edges[i].subPath[j]].rest_bw += aRequest.edges[i].subBW[j];
        // assert(edges[aRequest.edges[i].subPath[j]].rest_bw <= edges[aRequest.edges[i].subPath[j]].bw + EPSILON);
        edges[aRequest.edges[i].subPath[j]].count--;

        // remove request information
        iter1 = edges[aRequest.edges[i].subPath[j]].req_ids.begin();
        iter2 = edges[aRequest.edges[i].subPath[j]].edge_ids.begin();
        iter3 = edges[aRequest.edges[i].subPath[j]].used_bw.begin();
        while (iter1 != edges[aRequest.edges[i].subPath[j]].req_ids.end()) {
          if (*iter1 == aRequest.reqID && *iter2 == i) {
            edges[aRequest.edges[i].subPath[j]].req_ids.erase(iter1);
            edges[aRequest.edges[i].subPath[j]].edge_ids.erase(iter2);
            edges[aRequest.edges[i].subPath[j]].used_bw.erase(iter3);

            break;
          }
          iter1++;
          iter2++;
          iter3++;
        }
      }
    }
  }
}

double SubstrateGraph::getNodePotential(int nodeID) {
  // TODO: implement better node potential function

  return nodes[nodeID].rest_cpu;
}

int SubstrateGraph::findNodesWithinConstraints(Node &aNode, int reqID, int maxD, vector<int> &validNodeIDs) {
  int i, count = 0;
  validNodeIDs.clear();

  for (i = 0; i < nodeNum; i++) {
    if ((nodes[i].distanceFrom(aNode) <= maxD) && (nodes[i].rest_cpu >= aNode.cpu) && (nodes[i].touched == false)) {
      validNodeIDs.push_back(i);
      count++;
    }
  }

  return count;
}

int SubstrateGraph::initGraph() {
  int i;

  int x, y;
  double cpu;

  int from, to;
  double bw, dlay;

  fprintf(stderr, "initGraph: file: %s\n", fileName.c_str());

  FILE *fp = fopen(fileName.c_str(), "rt");

  if (!fp) {
    cout << "failed to open file: " << fileName << endl;
    return COULD_NOT_OPEN_FILE;
  }

  // formatting check
  int lines = 0;
  char* line = 0;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, fp)) != -1) {
      lines++;
  }
  free(line);
  fseek(fp, 0, SEEK_SET);

  fscanf(fp, "%d %d", &nodeNum, &edgeNum);

  if (lines != 1+nodeNum+edgeNum) {
      fprintf(stderr, "request file is not correctly formatted.\n");
      fprintf(stderr, "found: %d, needed %d\n", lines, 1+nodeNum+edgeNum);
      exit(1);
  }

  // read nodes and add them to vector
  for (i = 0; i < nodeNum; i++) {
    fscanf(fp, "%d %d %lf", &x, &y, &cpu);
    nodes.push_back(SubstrateNode(x, y, cpu));
    // printf("%d %d %lf\n", x, y, cpu);
    nodes[i].rest_cpu = cpu;
  }

  // read edges and add them to vector
  for (i = 0; i < edgeNum; i++) {
    fscanf(fp, "%d %d %lf %lf", &from, &to, &bw, &dlay);
    edges.push_back(SubstrateEdge(from, to, bw, dlay));
    // printf("%d %d %lf %lf\n", from, to, bw, dlay);
    edges[i].rest_bw = bw;

    // save edge information in nodes
    nodes[from].edgeIDs.push_back(i);
    nodes[to].edgeIDs.push_back(i);

    // record SN-SN neighbors. VN-SN will be added later.
    neighbor[from].push_back(to);
    neighbor[to].push_back(from);

    // save the edge map
    // (edge, ID)?
    edgeMap[make_pair(from, to)] = i;
    edgeMap[make_pair(to, from)] = i;
  }

  fclose(fp);

  return SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Class      : VNRequest                                                    //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

VNRequest::VNRequest(string _fileName) : fileName(_fileName), reqID(0) {
  if (DEBUG) {
    cout << "VNReq" << endl;
  }

  nodes.clear();
  edges.clear();

  initGraph();

  revenue = 0;
}

#ifdef MYCPP
VNRequest::VNRequest(const VNRequest &o) {
  if (DEBUG) {
    cout << "VNReq Copy" << endl;
  }

  nodeNum = o.nodeNum;
  edgeNum = o.edgeNum;
  reqID = o.reqID;
  fileName = o.fileName;
  revenue = o.revenue;

  split = o.split;
  time = o.time;
  duration = o.duration;
  topology = o.topology;
  maxD = o.maxD;

  // copy(o.nodes.begin(), o.nodes.end(), nodes.begin());
  for (int i = 0; i < o.nodeNum; i++) nodes.push_back(o.nodes[i]);
  // copy(o.edges.begin(), o.edges.end(), edges.begin());
  for (int i = 0; i < edgeNum; i++) edges.push_back(o.edges[i]);
}

VNRequest::~VNRequest() {
  if (DEBUG) {
    cout << "~VNReq" << endl;
  }
  nodes.clear();
  edges.clear();
}

const VNRequest &VNRequest::operator=(const VNRequest &o) {
  if (DEBUG) {
    cout << "VNReq=" << endl;
  }

  if (this != &o) {
    nodeNum = o.nodeNum;
    edgeNum = o.edgeNum;
    reqID = o.reqID;
    fileName = o.fileName;
    revenue = o.revenue;

    split = o.split;
    time = o.time;
    duration = o.duration;
    topology = o.topology;
    maxD = o.maxD;

    // copy(o.nodes.begin(), o.nodes.end(), nodes.begin());
    for (int i = 0; i < o.nodeNum; i++) nodes.push_back(o.nodes[i]);
    // copy(o.edges.begin(), o.edges.end(), edges.begin());
    for (int i = 0; i < edgeNum; i++) edges.push_back(o.edges[i]);
  }

  return *this;
}
#endif

int VNRequest::initGraph() {
  int i;

  int x, y;
  double cpu;

  int from, to;
  double bw, dlay;

  FILE *fp = fopen(fileName.c_str(), "rt");

  if (!fp) {
    cout << "failed to open file: " << fileName << endl;
    return COULD_NOT_OPEN_FILE;
  }

  // formatting check
  int lines = 0;
  char* line = 0;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, fp)) != -1) {
      lines++;
  }
  free(line);
  fseek(fp, 0, SEEK_SET);

  fscanf(fp, "%d %d %d %d %d %d %d", &nodeNum, &edgeNum, &split, &time, &duration, &topology, &maxD);

  if (lines != 1+nodeNum+edgeNum) {
      fprintf(stderr, "request file is not correctly formatted.\n");
      fprintf(stderr, "found: %d, needed %d\n", lines, 1+nodeNum+edgeNum);
      exit(1);
  }

  // read nodes
  for (i = 0; i < nodeNum; i++) {
    fscanf(fp, "%d %d %lf", &x, &y, &cpu);
    nodes.push_back(VNNode(x, y, cpu));
    // printf("%d %d %lf\n", x, y, cpu);
  }

  // read edges
  for (i = 0; i < edgeNum; i++) {
    fscanf(fp, "%d %d %lf %lf", &from, &to, &bw, &dlay);
    edges.push_back(VNEdge(from, to, bw, dlay));
    // printf("%d %d %lf %lf\n", from, to, bw, dlay);

    // save edge information in nodes
    nodes[from].edgeIDs.push_back(i);
    nodes[to].edgeIDs.push_back(i);
  }

  fclose(fp);

  return SUCCESS;
}

void VNRequest::sortNodesAscending(vector<int> &nodeProcessOrder) {
  int i, j;

  for (i = 0; i < nodeNum; i++) {
    for (j = i + 1; j < nodeNum; j++) {
      if (nodes[nodeProcessOrder[i]].cpu > nodes[nodeProcessOrder[j]].cpu) {
        int t = nodeProcessOrder[i];
        nodeProcessOrder[i] = nodeProcessOrder[j];
        nodeProcessOrder[j] = t;
      }
    }
  }
}

void VNRequest::sortNodesDescending(vector<int> &nodeProcessOrder) {
  int i, j;

  for (i = 0; i < nodeNum; i++) {
    for (j = i + 1; j < nodeNum; j++) {
      if (nodes[nodeProcessOrder[i]].cpu < nodes[nodeProcessOrder[j]].cpu) {
        int t = nodeProcessOrder[i];
        nodeProcessOrder[i] = nodeProcessOrder[j];
        nodeProcessOrder[j] = t;
      }
    }
  }
}

void VNRequest::sortEdgesAscending(vector<int> &edgeProcessOrder) {
  int i, j;

  for (i = 0; i < edgeNum; i++) {
    for (j = i + 1; j < edgeNum; j++) {
      if (edges[edgeProcessOrder[i]].bw > edges[edgeProcessOrder[j]].bw) {
        int t = edgeProcessOrder[i];
        edgeProcessOrder[i] = edgeProcessOrder[j];
        edgeProcessOrder[j] = t;
      }
    }
  }
}

void VNRequest::sortEdgesDescending(vector<int> &edgeProcessOrder) {
  int i, j;

  for (i = 0; i < edgeNum; i++) {
    for (j = i + 1; j < edgeNum; j++) {
      if (edges[edgeProcessOrder[i]].bw < edges[edgeProcessOrder[j]].bw) {
        int t = edgeProcessOrder[i];
        edgeProcessOrder[i] = edgeProcessOrder[j];
        edgeProcessOrder[j] = t;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Class      : SubstrateNode                                                //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

SubstrateNode::SubstrateNode(int _x, int _y, double _cpu) : Node(_x, _y, _cpu) {
  if (DEBUG) {
    cout << "SubNode" << endl;
  }

  count = 0;
  rest_cpu = 0;

  req_ids.clear();
  node_ids.clear();
  used_cpu.clear();
}

#ifdef MYCPP
SubstrateNode::SubstrateNode(const SubstrateNode &o) : Node(o), count(o.count), rest_cpu(o.rest_cpu) {
  if (DEBUG) {
    cout << "SubNode Copy" << endl;
  }

  edgeIDs.resize(o.edgeIDs.size());
  copy(o.edgeIDs.begin(), o.edgeIDs.end(), edgeIDs.begin());
  req_ids.resize(o.req_ids.size());
  copy(o.req_ids.begin(), o.req_ids.end(), req_ids.begin());
  node_ids.resize(o.node_ids.size());
  copy(o.node_ids.begin(), o.node_ids.end(), node_ids.begin());
  used_cpu.resize(o.used_cpu.size());
  copy(o.used_cpu.begin(), o.used_cpu.end(), used_cpu.begin());
}

SubstrateNode::~SubstrateNode() {
  if (DEBUG) {
    cout << "~SubNode" << endl;
  }
  edgeIDs.clear();
  req_ids.clear();
  node_ids.clear();
  used_cpu.clear();
}

const SubstrateNode &SubstrateNode::operator=(const SubstrateNode &o) {
  if (DEBUG) {
    cout << "SubNode=" << endl;
  }

  if (this != &o) {
    Node::operator=(o);

    count = o.count;
    rest_cpu = o.rest_cpu;

    edgeIDs.resize(o.edgeIDs.size());
    copy(o.edgeIDs.begin(), o.edgeIDs.end(), edgeIDs.begin());
    req_ids.resize(o.req_ids.size());
    copy(o.req_ids.begin(), o.req_ids.end(), req_ids.begin());
    node_ids.resize(o.node_ids.size());
    copy(o.node_ids.begin(), o.node_ids.end(), node_ids.begin());
    used_cpu.resize(o.used_cpu.size());
    copy(o.used_cpu.begin(), o.used_cpu.end(), used_cpu.begin());
  }

  return *this;
}
#endif

double SubstrateNode::distanceFrom(Node &aNode) {
  return sqrt((x - aNode.x) * (x - aNode.x) + (y - aNode.y) * (y - aNode.y));
}

double SubstrateNode::distanceFrom(SubstrateNode &aNode) {
  return sqrt((x - aNode.x) * (x - aNode.x) + (y - aNode.y) * (y - aNode.y));
}

///////////////////////////////////////////////////////////////////////////////
// Class      : SubstrateEdge                                                //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

SubstrateEdge::SubstrateEdge(int _from, int _to, double _bw, double _dlay) : Edge(_from, _to, _bw, _dlay) {
  if (DEBUG) {
    cout << "SubEdge" << endl;
  }
  count = 0;
  rest_bw = 0;

  req_ids.clear();
  edge_ids.clear();
  used_bw.clear();
}

#ifdef MYCPP
SubstrateEdge::SubstrateEdge(const SubstrateEdge &o) : Edge(o), count(o.count), rest_bw(o.rest_bw) {
  if (DEBUG) {
    cout << "SubEdge Copy" << endl;
  }

  req_ids.resize(o.req_ids.size());
  copy(o.req_ids.begin(), o.req_ids.end(), req_ids.begin());
  edge_ids.resize(o.edge_ids.size());
  copy(o.edge_ids.begin(), o.edge_ids.end(), edge_ids.begin());
  used_bw.resize(o.used_bw.size());
  copy(o.used_bw.begin(), o.used_bw.end(), used_bw.begin());
}

SubstrateEdge::~SubstrateEdge() {
  if (DEBUG) {
    cout << "~SubEdge" << endl;
  }
  req_ids.clear();
  edge_ids.clear();
  used_bw.clear();
}

const SubstrateEdge &SubstrateEdge::operator=(const SubstrateEdge &o) {
  if (DEBUG) {
    cout << "SubEdge=" << endl;
  }
  if (this != &o) {
    Edge::operator=(o);

    count = o.count;
    rest_bw = o.rest_bw;

    req_ids.resize(o.req_ids.size());
    copy(o.req_ids.begin(), o.req_ids.end(), req_ids.begin());
    edge_ids.resize(o.edge_ids.size());
    copy(o.edge_ids.begin(), o.edge_ids.end(), edge_ids.begin());
    used_bw.resize(o.used_bw.size());
    copy(o.used_bw.begin(), o.used_bw.end(), used_bw.begin());
  }

  return *this;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Class      : VNNode                                                       //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

VNNode::VNNode(int _x, int _y, double _cpu) : Node(_x, _y, _cpu) {
  if (DEBUG) {
    cout << "VNNode" << endl;
  }

  substrateID = NOT_MAPPED_YET;
  subNodeID = NOT_MAPPED_YET;
}

#ifdef MYCPP
VNNode::VNNode(const VNNode &o) : Node(o), substrateID(o.substrateID), subNodeID(o.subNodeID) {
  if (DEBUG) {
    cout << "VNNode Copy" << endl;
  }

  edgeIDs.resize(o.edgeIDs.size());
  copy(o.edgeIDs.begin(), o.edgeIDs.end(), edgeIDs.begin());
}

VNNode::~VNNode() {
  if (DEBUG) {
    cout << "~VNNode" << endl;
  }

  edgeIDs.clear();
}

const VNNode &VNNode::operator=(const VNNode &o) {
  if (DEBUG) {
    cout << "VNNode=" << endl;
  }

  if (this != &o) {
    Node::operator=(o);

    substrateID = o.substrateID;
    subNodeID = o.subNodeID;

    edgeIDs.resize(o.edgeIDs.size());
    copy(o.edgeIDs.begin(), o.edgeIDs.end(), edgeIDs.begin());
  }

  return *this;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Class      : VNEdge                                                       //
// Description:                                                              //
///////////////////////////////////////////////////////////////////////////////

VNEdge::VNEdge(int _from, int _to, double _bw, double _dlay) : Edge(_from, _to, _bw, _dlay) {
  if (DEBUG) {
    cout << "VNEdge" << endl;
  }

  pathLen = 0;
  pathDelay = NOT_MAPPED_YET;

  substrateID = NOT_MAPPED_YET;
  subPath.clear();
  subBW.clear();
}

#ifdef MYCPP
VNEdge::VNEdge(const VNEdge &o) : Edge(o), substrateID(o.substrateID), pathLen(o.pathLen), pathDelay(o.pathDelay) {
  if (DEBUG) {
    cout << "VNEdge Copy" << endl;
  }

  subPath.resize(o.subPath.size());
  copy(o.subPath.begin(), o.subPath.end(), subPath.begin());
  subBW.resize(o.subBW.size());
  copy(o.subBW.begin(), o.subBW.end(), subBW.begin());
}

VNEdge::~VNEdge() {
  if (DEBUG) {
    cout << "~VNEdge" << endl;
  }

  subPath.clear();
  subBW.clear();
}

const VNEdge &VNEdge::operator=(const VNEdge &o) {
  if (DEBUG) {
    cout << "VNEdge=" << endl;
  }

  if (this != &o) {
    Edge::operator=(o);

    substrateID = o.substrateID;
    pathLen = o.pathLen;
    pathDelay = o.pathDelay;

    subPath.resize(o.subPath.size());
    copy(o.subPath.begin(), o.subPath.end(), subPath.begin());
    subBW.resize(o.subBW.size());
    copy(o.subBW.begin(), o.subBW.end(), subBW.begin());
  }

  return *this;
}
#endif
