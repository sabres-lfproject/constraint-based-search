/*
 * simulator.h
 *
 *  Created on: 14-Jul-2008
 *      Author: nmmkchow
 */

#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_

#include <queue>

#include "def.hpp"
#include "utility.hpp"

using namespace std;

#define EVENT_ARRIVE 0
#define EVENT_DEPART 1
#define EVENT_END_SIM 2

class Event;
class Simulator;

// Event object
class Event {
 public:
  int type, time, index;

  Event(int _type, int _time, int _index);
  virtual ~Event();
};

// How to compare event objects
class EventComparer {
 public:
  bool operator()(const Event& e1, const Event& e2) const { return (e1.time > e2.time); }
};

class Simulator {
 public:
  // a queue for holding events.
  priority_queue<Event, vector<Event>, EventComparer> PQ;

  const Event& top();
  void pop();
  bool empty();

  Simulator();
  virtual ~Simulator();
};

#endif /* SIMULATOR_HPP_ */
