// VNE-CBS code.
//
// Code was built based on the ViNE-Yard simulator from the paper:
// Chowdhury, N.M.K., Rahman, M.R. and Boutaba, R., 2009, April.
// Virtual network embedding with coordinated node and link mapping. In IEEE INFOCOM 2009 (pp. 783-791). IEEE.
//

#include "cbs.hpp"
#include "def.hpp"
#include "simulator.hpp"

int main(int argc, char *argv[]) {
  int i;
  bool aOne = false, bOne = false;
  double mNS, aNS, mLS, aLS, sdNS, sdLS;
  double __MULT = 1.0;
  double sub_w = 1.0;
  int nodeMapMethod;
  double nodeRev, edgeRev, totRev, nodeCost, edgeCost, totCost;
  fprintf(stderr, "%d\n", argc);
  if (argc != 4) {
    cerr << "usage: <req> <sub> <outDir>" << endl;
    cerr << "<req>: the virtual network request file path" << endl;
    cerr << "<sub>: the substrate network file path" << endl;
    cerr << "<outDir>: output directory to dump the results" << endl;
    exit(1);
  }

  std::string reqFileName = argv[1];
  std::string sn_graph_path = argv[2];
  std::string outputDirectory = argv[3];

  nodeMapMethod = 7;      // 1: GREEDY. 5: D-ViNE 6: R-ViNE 7:VNE-CBS
  aOne = bOne = 0;        // true: try to load balance
  __MULT = 1.0;           // cpu vs bw weighting value
  // load network graph.
  SubstrateGraph SG(sn_graph_path, 0);

  // init simulator
  Simulator mySim;

  // init CBS solver
  CBS_Solver cbs;

  std::string outputFileName, outputMapName;
  outputFileName = outputDirectory + "/" + "out.txt";
  outputMapName = outputDirectory +  "/" + "out.json";

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

  int mapFailCount = 0;

  srand((unsigned)time(NULL));  // initialize random number generator

  VNRequest VNR = VNRequest(reqFileName.c_str());

  // create the arrival event and add to event queue.
  mySim.PQ.push(Event(EVENT_ARRIVE, VNR.time, 0));
  cout << "event added." << endl;

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
        vector<Path> mapping = cbs.find_solution(SG, VNR, sub_w);
        if (mapping.empty()) {
          mapFailCount++;
          goto LABEL_MAP_FAILED;
        } else {
          // update VNR info, CPU, BW etc.
          cout << "Updating VNR info" << endl;
          cbs.update_VNR_info(mapping, VNR, SG);
        }

      } else {
        cerr << "Invalid Parameter" << endl;
        mapFailCount++;
        goto LABEL_MAP_FAILED;
      }

      requestAccepted = true;
      SG.addVNMapping(VNR);

      printMapping(VNR, SG);
      saveMapping(VNR, SG, outputMapName);
      //      SG.printNodeStatus();
      //      SG.printEdgeStatus();
      // create the departure event after admitting a VN request
      mySim.PQ.push(Event(EVENT_DEPART, VNR.time + VNR.duration, curVNR));
    } else if (curEvent.type == EVENT_DEPART) {  // handle departure event
      SG.removeVNMapping(VNR);
    } else {
    }

  LABEL_MAP_FAILED:
    totRev = getRevenue(VNR, __MULT, nodeRev, edgeRev);
    totCost = getCost(VNR, SG, __MULT, nodeCost, edgeCost, aOne, bOne);
    getDifferentStress(SG, mNS, aNS, mLS, aLS, sdNS, sdLS);

    auto end = chrono::high_resolution_clock::now();
    cout << "totRev: " << totRev << " total Cost: " << totCost << endl;
    cout << "runtime: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0 << endl;
    fprintf(outputFP,
            "%4d, %6d, %6d, %d, %d, %10.4lf, %10.4lf, %d, %.4f\n",
            curEvent.index,
            curEvent.time,
            VNR.duration,
            curEvent.type,
            requestAccepted,
            totRev,
            totCost,
            cbs.count_num,
            chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0);
    // pop the request
    mySim.pop();
  }

  cout << "Mapping Failed: " << mapFailCount << endl;

  fclose(outputFP);

  exit(mapFailCount);
}
