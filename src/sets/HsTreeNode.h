/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_SETS_HSTREENODE_H_
#define FSM_SETS_HSTREENODE_H_

#include <string>
#include <unordered_set>
#include <vector>

class HsTreeNode
{
private:
	std::unordered_set<int> x;
	std::vector<std::unordered_set<int>> s;
	std::vector<HsTreeNode> children;
	int nodeNum;
public:
	/**
	 * The smallest hitting set
	 */
	static std::unordered_set<int> hSmallest;

	/**
	 *The number of hitting set tree nodes already created
	 */
	static int maxNodeNum;

	//TODO
	HsTreeNode(const std::unordered_set<int>& x, const std::vector<std::unordered_set<int>>& s);

	/**
	 * Getter for the size of the hitting set
	 * @return The size of the hitting set
	 */
	size_t size() const;

	/**
	 * Check whether or not, it is a hitting set
	 * @return true if is a hitting set, false otherwise
	 */
	bool isHittingSet() const;

	/**
	 * Add a new hitting set tree node to this node's children
	 * @param The node to add
	 */
	void add(const HsTreeNode & node);
	
	/** 
     *  Create children of the current node, such that
     *  Each child has the elements of the current node
     *  minus 1 element, and each child is again a hitting set.
     *  Each child obviously is a better candidate for the
     *  minimal hitting set than the current node, since
     *  its cardinality is that of the current node minus one.
     *  Note that it may be the case that no children are
     *  created at all, because every subset of this node is
     *  no longer a hitting set.
     */
	void expandNode();

	/**
	 * Create a dot file representing the hitting set (by calling this 
     * method recursively on its children)
	 * @return A string containing the representation
	*/
	std::string toDot();
};
#endif //FSM_SETS_HSTREENODE_H_
