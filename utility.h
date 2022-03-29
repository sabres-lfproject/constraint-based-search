/*
 * Utility.h
 *
 *  Created on: Jul 13, 2008
 *      Author: nmmkchow
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <cmath>

#include <iostream>
#include <fstream>

#include <deque>
#include <vector>
#include <map>

#include "def.h"

using namespace std;

// These classes are predefined.

/**
 * Node: Node(int _x, int _y, double _cpu)
 * Edge: Edge(int _from, int _to, double _bw, double _dlay)
 */
class Node;
class Edge;

/**
 * SubstrateNode(int _x, int _y, double _cpu), count = 0; rest_cpu = 0;
 * SubstrateEdge(int _from, int _to, double _bw, double _dlay) count = 0; rest_bw = 0;
 * SubstrateGraph(string _fileName, int _substrateID)
 */
class SubstrateNode;
class SubstrateEdge;
class SubstrateGraph;

class VNNode;
class VNEdge;
class VNRequest;

// function declarations
void printMapping(VNRequest &VNR, SubstrateGraph &SG);
void saveMapping(VNRequest &VNR, SubstrateGraph &SG, string save_file_path,int VNR_id);
void randomPermutation(vector<int> &series);

void getDifferentStress(SubstrateGraph &SG, double &mNS, double &aNS,
    double &mLS, double &aLS, double &sdNS, double &sdLS);

double getRevenue(VNRequest &aRequest, double __MULT,
    double &nodeRev, double &edgeRev);

double getCost(VNRequest &VNR, SubstrateGraph &SG, double __MULT,
    double &nodeCost, double &edgeCost, bool aOne, bool bOne);

class Node {
public:
  int x, y;
  double cpu;

  Node(int _x, int _y, double _cpu);

#ifdef MYCPP
  Node(const Node &o);
  virtual ~Node();
  const Node& operator=(const Node &o);
#endif
};

class Edge {
public:
  int from, to;
  double bw, dlay;

  Edge(int _from, int _to, double _bw, double _dlay);

#ifdef MYCPP
  Edge(const Edge &o);
  virtual ~Edge();
  const Edge& operator=(const Edge &o);
#endif
};

class SubstrateNode: public Node {
public:
  int count;  // this is id?
  double rest_cpu;  // rest of resources
  bool touched;   // what is this?

  vector<int> edgeIDs;    // which edges are related to this node?

  vector<int> req_ids;    // which reqs are using this node?
  vector<int> node_ids;    // what's this?
  vector<double> used_cpu;

  SubstrateNode(int _x, int _y, double _cpu);

#ifdef MYCPP
  SubstrateNode(const SubstrateNode &o);
  virtual ~SubstrateNode();
  const SubstrateNode& operator=(const SubstrateNode &o);
#endif

  double distanceFrom(Node &aNode);
  double distanceFrom(SubstrateNode &aNode);
};

class SubstrateEdge: public Edge {
public:
  int count;
  double rest_bw;

  vector<int> req_ids;
  vector<int> edge_ids;
  vector<double> used_bw;

  SubstrateEdge(int _from, int _to, double _bw, double _dlay);

#ifdef MYCPP
  SubstrateEdge(const SubstrateEdge &o);
  virtual ~SubstrateEdge();
  const SubstrateEdge& operator=(const SubstrateEdge &o);
#endif
};

class SubstrateGraph {
public:
  string fileName;
  int nodeNum, edgeNum, substrateID; // what ID?

  vector<SubstrateNode> nodes;
  vector<SubstrateEdge> edges;

  map< pair<int, int>, int > edgeMap;  // ?
  map<int, vector<int>> neighbor;
  // add reqs and remove reqs (meta nodes/edges)?
  void addVNMapping(VNRequest &aRequest);
  void removeVNMapping(const VNRequest &aRequest);

  SubstrateGraph(string _fileName, int _substrateID = -1);

#ifdef MYCPP
  SubstrateGraph(const SubstrateGraph &o);
  virtual ~SubstrateGraph();
  const SubstrateGraph& operator=(const SubstrateGraph &o);
#endif

  int initGraph();

  // Mapping VN to SN nodes?
  int findNodesWithinConstraints(Node &aNode, int reqID, int maxD,
      vector<int> &validNodeIDs);

  double getNodePotential(int nodeID);

  void printNodeStatus();
  void printEdgeStatus();

  SubstrateGraph(SubstrateGraph & graph){
      fileName = graph.fileName;
      nodeNum = graph.nodeNum;
      edgeNum = graph.edgeNum;
      substrateID = graph.substrateID;

      // I hope these are deep copy
      nodes = graph.nodes;
      edges = graph.edges;
      neighbor = graph.neighbor;
      edgeMap = graph.edgeMap;
  }
};

class VNNode: public Node {
public:
  int substrateID;
  int subNodeID;

  vector<int> edgeIDs;

  VNNode(int _x, int _y, double _cpu);

#ifdef MYCPP
  VNNode(const VNNode &o);
  virtual ~VNNode();
  const VNNode& operator=(const VNNode &o);
#endif
};

class VNEdge: public Edge {
public:
  int substrateID;
  int pathLen;
  double pathDelay;
  vector < int> subPath;
  vector <double> subBW;

  VNEdge(int _from, int _to, double _bw, double _dlay);

#ifdef MYCPP
  VNEdge(const VNEdge &o);
  virtual ~VNEdge();
  const VNEdge& operator=(const VNEdge &o);
#endif
};

class VNRequest {
public:
  string fileName;
  int split, time, duration, topology, maxD;
  double revenue;
  int nodeNum, edgeNum, reqID;

  vector<VNNode> nodes;
  vector<VNEdge> edges;

  VNRequest(string _fileName, int _reqID);

#ifdef MYCPP
  VNRequest(const VNRequest &o);
  virtual ~VNRequest();
  const VNRequest& operator=(const VNRequest &o);
#endif

  int initGraph();

  void sortNodesAscending(vector<int> &nodeProcessOrder);
  void sortNodesDescending(vector<int> &nodeProcessOrder);

  void sortEdgesAscending(vector<int> &edgeProcessOrder);
  void sortEdgesDescending(vector<int> &edgeProcessOrder);
};

class VNEdgeComparerASC {
public:
  bool operator()(const VNEdge& n1, const VNEdge& n2) const {
    return (n1.bw < n2.bw);
  }
};

class VNEdgeComparerDESC {
public:
  bool operator()(const VNEdge& n1, const VNEdge& n2) const {
    return (n1.bw > n2.bw);
  }
};

#endif /* UTILITY_H_ */
