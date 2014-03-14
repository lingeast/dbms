/*
 * floatkey.cpp
 *
 *  Created on: Mar 1, 2014
 *      Author: yidong
 */

#include "floatkey.h"
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <stdexcept>

float_key::float_key() : pos_inf(false), neg_inf(false), len(0), val(0) {
	// TODO Auto-generated constructor stub

}

float_key::float_key(const float_key & that): pos_inf(that.pos_inf), neg_inf(that.neg_inf),
		len(that.len), val(that.val) {}

float_key::~float_key() {
	// TODO Auto-generated destructor stub
}

void float_key::load(const void* data) {
	memcpy(&val, data, sizeof(float_key_t));
	len = sizeof(float_key_t);
	//float val = *(float*)data;
	//std::cout << "Read in " << val << std::endl;
}

std::string float_key::to_string() const {
	char buffer [33];
	float tmp;
	memcpy(&tmp, &(this->val), sizeof(this->val));
	sprintf(buffer,"%f",tmp);
	return std::string(buffer);
}

bool float_key::operator<(const float_key &that) {
	//std::cout << "In < op" << std::endl;
	if (sizeof(float) == sizeof(val)) {
		float lhs, rhs;
		memcpy(&lhs, &val, sizeof(lhs));
		memcpy(&rhs, &(that.val), sizeof(rhs));
		//std::cout << "Compare " << lhs <<" and " << rhs << std::endl;
		return lhs < rhs;
	} else if (sizeof(double) == sizeof(val)) {
		double lhs, rhs;
		memcpy(&lhs, &val, sizeof(lhs));
		memcpy(&rhs, &(that.val), sizeof(rhs));
		return lhs < rhs;
	} else {
		return false; // fail to compare
	}

}
bool float_key::operator==(const float_key &that) {
	if (sizeof(float) == sizeof(val)) {
		float lhs, rhs;
		memcpy(&lhs, &val, sizeof(lhs));
		memcpy(&rhs, &(that.val), sizeof(rhs));
		return lhs == rhs;
	} else if (sizeof(double) == sizeof(val)) {
		double lhs, rhs;
		memcpy(&lhs, &val, sizeof(lhs));
		memcpy(&rhs, &(that.val), sizeof(rhs));
		return lhs == rhs;
	} else {
		return false; // fail to compare
	}
}

bool float_key::operator<(const comparable &rhs) {
    const float_key *pRhs = dynamic_cast<const float_key *>(&rhs);
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
    		return *this < *pRhs;
    	}
    }
}
bool float_key::operator==(const comparable &rhs) {
    const float_key *pRhs = dynamic_cast<const float_key *>(&rhs);
    if (pRhs == NULL) { // rhs is of another type
    	return false;
    } else {
    	// no equality on infinity
    	if (this->neg_inf || this->pos_inf || pRhs->neg_inf || pRhs->pos_inf) return false;
    	return (*this == *pRhs);
    }
}

void float_key::set_inf(int flag) {
	if (flag == 0) {
		pos_inf = false;
		neg_inf =false;
		return;
	}

	if (is_inf())
		throw new std::logic_error("Infinity already been set");
	if (flag > 0) pos_inf = true;
	if (flag < 0) neg_inf = true;
	return;
}
