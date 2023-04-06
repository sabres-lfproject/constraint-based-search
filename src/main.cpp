// VNE-CBS code.
//
// Code was built based on the ViNE-Yard simulator from the paper:
// Chowdhury, N.M.K., Rahman, M.R. and Boutaba, R., 2009, April.
// Virtual network embedding with coordinated node and link mapping. In IEEE INFOCOM 2009 (pp. 783-791). IEEE.
//

#include "CBS.h"
#include "def.h"
#include "simulator.h"

using namespace std;

#define CBS_solver false

int main(int argc, char *argv[]) {
  int reqCount;

  int i;
  bool aOne = false, bOne = false;
  double mNS, aNS, mLS, aLS, sdNS, sdLS;
  double __MULT = 1.0;
  double sub_w = 1.0;
  int nodeMapMethod;
  double nodeRev, edgeRev, totRev, nodeCost, edgeCost, totCost;
  string sn_graph_path;
  if (argc != 7) {
    cerr << "usage: SIM <rC> <rSub> <rD> <oF> <oM> <nM> <eM> <fS> <lB> <MT>" << endl;
    cerr << "<rC>: total number of requests" << endl;
    cerr << "<rSub>: the substrate network file path" << endl;
    cerr << "<rD>: directory containing the requests " << endl;
    cerr << "<oF>: output file to dump the results" << endl;
    cerr << "<oM>: where to output the mapping" << endl;
    //    cerr << "<nM>: node mapping method (7: VNE-CBS)" << endl;
    //    cerr << "<eM>: edge mapping method (Not supported: this version is for VNE-CBS only. Use default value 0)" <<
    //    endl; cerr << "<fS>: whether to ignore (1) or respect(0) VNR's splitting choice [eM must be 5]" << endl; cerr
    //    << "<lB>: load balancing (1: alpha = beta = 1. 0: alpha = beta = residue)" << endl; cerr << "<MT>: multiplier
    //    in the revenue and cost functions" << endl;
    cerr << "<w>: suboptimal bound" << endl;
    exit(1);
  }

  reqCount = atoi(argv[1]);  // total number of requests
  sn_graph_path = argv[3];

  std::string reqFolderName = argv[2];
  std::string outputFileName = argv[4];

  string mapping_save_to = argv[5];

  nodeMapMethod = 7;      // 1: GREEDY. 5: D-ViNE 6: R-ViNE 7:VNE-CBS
  aOne = bOne = 0;        // true: try to load balance
  __MULT = 1.0;           // cpu vs bw weighting value
  sub_w = atof(argv[6]);  // suboptimal bound for cbs.
  // load network graph.
  SubstrateGraph SG(sn_graph_path, 0);

  // vetor stores VNR requests.
  vector<VNRequest> VNR;

  // init simulator
  Simulator mySim;

  // init CBS solver
  CBS_Solver cbs;

  // output file opening.
  FILE *outputFP = fopen(outputFileName.c_str(), "w");
  if (outputFP == NULL) {
    cout << "failed to open file: " << outputFileName << endl;
    return COULD_NOT_OPEN_FILE;
  }
  // output command.
  for (i = 1; i < argc; i++) {
    fprintf(outputFP, "%s ", argv[i]);
  }
  fprintf(outputFP, "\n");

  // VNR vector
  vector<VNRequest>::iterator VNRIter;
  int curVNR;
  bool requestAccepted = false;

  int nodeMapFailCount = 0, edgeMapFailCount = 0, totalMapFailCount = 0;

  srand((unsigned)time(NULL));  // initialize random number generator

  // read all the requests, one by one.
  for (i = 0; i < reqCount; i++) {
    std::string reqFileName = reqFolderName + "/req" + std::to_string(i);
    // sprintf(reqFileName, "%s/req%d.txt", reqFolderName, i);

    // save the request in the list.
    VNR.push_back(VNRequest(reqFileName.c_str(), i));

    // create the arrival event and add to event queue.
    mySim.PQ.push(Event(EVENT_ARRIVE, VNR[i].time, i));
    cout << "pushed." << endl;
  }

  // simulate all the events
  while (!mySim.empty()) {
    // get top new event
    const Event &curEvent = mySim.top();
    curVNR = curEvent.index;

    requestAccepted = false;

    cout << curVNR << " " << curEvent.time << " " << curEvent.type << endl;
    auto start = chrono::high_resolution_clock::now();
    // mapping for new event.
    if (curEvent.type == EVENT_ARRIVE) {  // handle arrival of a new VN request

      if (nodeMapMethod == 7) {
        cout << "New Event ---------" << endl;
        cbs.count_num = 0;
        vector<Path> mapping = cbs.find_solution(SG, VNR[curVNR], sub_w);
        if (mapping.empty()) {
          totalMapFailCount++;
          goto LABEL_MAP_FAILED;
        } else {
          // update VNR info, CPU, BW etc.
          cout << "Updating VNR info" << endl;
          cbs.update_VNR_info(mapping, VNR[curVNR], SG);
        }

      } else {
        cerr << "Invalid Parameter" << endl;
        totalMapFailCount++;
        goto LABEL_MAP_FAILED;
      }

      requestAccepted = true;
      SG.addVNMapping(VNR[curVNR]);

      printMapping(VNR[curVNR], SG);
      saveMapping(VNR[curVNR], SG, mapping_save_to, curVNR);
      //      SG.printNodeStatus();
      //      SG.printEdgeStatus();
      // create the departure event after admitting a VN request
      mySim.PQ.push(Event(EVENT_DEPART, VNR[curVNR].time + VNR[curVNR].duration, curVNR));
    } else if (curEvent.type == EVENT_DEPART) {  // handle departure event
      SG.removeVNMapping(VNR[curVNR]);
    } else {
    }

  LABEL_MAP_FAILED:
    totRev = getRevenue(VNR[curVNR], __MULT, nodeRev, edgeRev);
    totCost = getCost(VNR[curVNR], SG, __MULT, nodeCost, edgeCost, aOne, bOne);
    getDifferentStress(SG, mNS, aNS, mLS, aLS, sdNS, sdLS);

    auto end = chrono::high_resolution_clock::now();
    cout << "totRev: " << totRev << " total Cost: " << totCost << endl;
    cout << "runtime: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0 << endl;
    fprintf(outputFP,
            "%4d, %6d, %6d, %d, %d, %10.4lf, %10.4lf, %d, %.4f\n",
            curEvent.index,
            curEvent.time,
            VNR[curEvent.index].duration,
            curEvent.type,
            requestAccepted,
            totRev,
            totCost,
            cbs.count_num,
            chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0);
    // pop the request
    mySim.pop();
  }

  cout << "Node Mapping Failed: " << nodeMapFailCount << endl;
  cout << "Edge Mapping Failed: " << edgeMapFailCount << endl;
  cout << "Mapping Failed: " << totalMapFailCount << endl;

  fclose(outputFP);

  return 0;
}
