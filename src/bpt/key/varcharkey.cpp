/*
 * varcharkey.cpp
 *
 *  Created on: Feb 27, 2014
 *      Author: yidong
 */

#include "varcharkey.h"
#include <iostream>
#include <cstring>
#include <stdint.h>
#include <stdexcept>

varchar_key::varchar_key() : pos_inf(false), neg_inf(false), raw_data(NULL), len(0), str() {}

varchar_key::varchar_key(const varchar_key &that) : pos_inf(that.pos_inf), neg_inf(that.neg_inf),
		raw_data(NULL), len(that.len), str(that.str) {
	this->raw_data = new char[this->len];
	memcpy(this->raw_data, that.raw_data, this->len);
}

varchar_key::~varchar_key() {
	if (raw_data != NULL)
		delete raw_data;
}
size_t varchar_key::length() const {
	return len;
}

const void* varchar_key::data() const {
	return (void*) raw_data;
}

	// load from memory, serialzable_ptr->load(ptr_to_position);
void varchar_key::load(const void *out_data) {
	if (raw_data != NULL) delete raw_data;
	int strlen = *(uint32_t*)out_data;
	this->len = strlen + sizeof(uint32_t);
	raw_data = new char[this->len];
	memcpy(raw_data, out_data, strlen + sizeof(uint32_t));

	string str1(raw_data + sizeof(uint32_t), strlen);
	//string str1("made");
	//::cout << str1 << std::endl;
	str = str1;
}

	// Used for debug
std::string varchar_key::to_string() const {
	return str;
}


bool varchar_key::operator<(const varchar_key & that) {
	return this->str < that.str;
}

bool varchar_key::operator==(const varchar_key & that) {
	return this->str == that.str;
}

bool varchar_key::operator<(const comparable &rhs) {
    const varchar_key *pRhs = dynamic_cast<const varchar_key *>(&rhs);
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
     		return (str.compare(pRhs->str) < 0);
     	}
     }
    return false;
}

bool varchar_key::operator==(const comparable &rhs)  {
    const varchar_key *pRhs = dynamic_cast<const varchar_key *>(&rhs);
    if (pRhs == NULL) { // rhs is of another type
         return false;
     } else {
    	 // No equality between infinity
    	 if (this->neg_inf || this->pos_inf || pRhs->neg_inf || pRhs->pos_inf) return false;
    	 return this->str == pRhs->str;
     }
}

void varchar_key::set_inf(int flag) {
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



