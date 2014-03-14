/*
 * intkey.cpp
 *
 *  Created on: Mar 1, 2014
 *      Author: yidong
 */

#include "intkey.h"
#include <stdio.h>
#include <iostream>
#include <stdexcept>
int_key::int_key(): pos_inf(false), neg_inf(false), val(0), len(0) {
	// TODO Auto-generated constructor stub
}

int_key::int_key(const int_key& that) :pos_inf(that.pos_inf), neg_inf(that.neg_inf),
		val(that.val) , len(that.len){
}

int_key::~int_key() {
	// TODO Auto-generated destructor stub
}

void int_key::load(const void* data) {
	this->val = *(int_key_t*)data;
	len = sizeof(int_key_t);
}

std::string int_key::to_string() const {
	char buffer [33];
	sprintf(buffer,"%d",this->val);
	if (this->val == 0) {
		std::cout << "Impossible!";
	}
	return std::string(buffer);
}

void int_key::set_inf(int flag) {
	if (flag == 0) {
		pos_inf = false;
		neg_inf =false;
		return;
	}

	if (is_inf())
		throw new std::logic_error("Infinity already been set");
	if (flag > 0) pos_inf = true;
	else if (flag < 0) neg_inf = true;
}

bool int_key::operator <(const comparable &rhs) {
    const int_key *pRhs = dynamic_cast<const int_key *>(&rhs);
    if (pRhs == NULL) { // rhs is of another type
    	return false;
    } else {
    	if (this->is_inf() && pRhs->is_inf()) {
    		throw new std::logic_error("Comparing infinites is useless");
    		return false;
    	} else if (this->is_inf()) {
    		if (pos_inf) return false;
    		if (neg_inf) return true;
    	} else if (pRhs->is_inf()) {
    		if (pRhs->pos_inf) return true;
    		if (pRhs->neg_inf) return false;
    	} else {
    		return this->val < pRhs->val;
    	}
    }
    return false;
}

bool int_key::operator ==(const comparable & rhs) {
    const int_key *pRhs = dynamic_cast<const int_key *>(&rhs);
    if (pRhs == NULL) { // rhs is of another type
    	return false;
    } else {
    	// No equality between infinity
    	if (this->neg_inf || this->pos_inf || pRhs->neg_inf || pRhs->pos_inf) return false;
    	return (this->val == pRhs->val);
    }
}
