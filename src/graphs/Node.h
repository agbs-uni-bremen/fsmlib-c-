/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 *
 * Licensed under the EUPL V.1.1
 */

#ifndef FSM_GRAPHS_NODE_H_
#define FSM_GRAPHS_NODE_H_

#include <vector>
#include "graphs/Edge.h"

using namespace std;

class Node {
protected:

    /**
    * id of this node
    */
    int id;

    /**
    * A list containing the ingoing edges of this node.
    */
    vector<weak_ptr<Edge>> inEdges;


    /**
    * A list containing the outgoing edges of this node.
    */
    vector<shared_ptr<Edge>> edges;

public:

    /**
    * Create a new node
    * @param id the ID of this node
    */
    Node(const int id);

    /**
    * Add an outgoing edge to this node
    * @param edge The outgoing edge to be added
    */
    void addEdge(const shared_ptr<Edge>& edge);

    /**
    * Add an ingoing edge to this node
    * @param inEdge The ingoing edge to be added
    */
    void addInEdge(const shared_ptr<Edge>& inEdge);

    /**
    * Gets the id of this node
    * @return the id of this node
    */
    int getId();

};
#endif //FSM_GRAPHS_NODE_H_