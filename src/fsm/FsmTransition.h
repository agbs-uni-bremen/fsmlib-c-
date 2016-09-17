/*
 * Copyright. Gaël Dottel, Christoph Hilken, and Jan Peleska 2016 - 2021
 * 
 * Licensed under the EUPL V.1.1
 */
#ifndef FSM_FSM_FSMTRANSITION_H_
#define FSM_FSM_FSMTRANSITION_H_

#include <memory>

#include "fsm/FsmLabel.h"

class FsmNode;

class FsmTransition
{
private:
	/**
	The node from which the transition come
	*/
	std::weak_ptr<FsmNode> source;

	/**
	The node where the transition go
	*/
	std::weak_ptr<FsmNode> target;

	/**
	The label of this transition
	*/
	FsmLabel label;
public:
	/**
	Create a FsmTransition
	\param source The node from which the transition come
	\param target The node where the transition go
	\param label The label of this transition
	*/
	FsmTransition(const std::shared_ptr<FsmNode> source, const std::shared_ptr<FsmNode> target, const FsmLabel & label);

	/**
	Getter for the source
	\return The node from which the transition come
	*/
	std::shared_ptr<FsmNode> getSource() const;

	/**
	Getter for the target
	\return The node where the transition go
	*/
	std::shared_ptr<FsmNode> getTarget() const;

	/**
	Getter for the label
	\return The label of this transition
	*/
	FsmLabel getLabel() const;

	/**
	Output the FsmTransition to a standard output stream
	\param out The standard output stream to use
	\param transition The FsmTransition to print
	\return The standard output stream used, to allow user to cascade <<
	*/
	friend std::ostream & operator<<(std::ostream & out, const FsmTransition & transition);
};
#endif //FSM_FSM_FSMTRANSITION_H_