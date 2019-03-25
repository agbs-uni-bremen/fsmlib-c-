/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */

#ifndef FSM_GRAPHS_GRAPH_H_
#define FSM_GRAPHS_GRAPH_H_

#include <vector>
#include "graphs/Node.h"

using namespace std;

class Graph {
protected:

    /**
    * The nodes of this graph
    */
    vector<shared_ptr<Node>> nodes;

public:

    /**
    * Creates a new graph
    * @param nodes the nodes of this graph
    */
    Graph(const vector<shared_ptr<Node>>& nodes);
};

#endif //FSM_GRAPHS_GRAPH_H_