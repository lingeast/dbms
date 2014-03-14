/*
 * floatkey.h
 *
 *  Created on: Mar 1, 2014
 *      Author: yidong
 */

#ifndef FLOATKEY_H_
#define FLOATKEY_H_

#include "btkey.h"
#include <stdint.h>

typedef int32_t float_key_t;

class float_key: public BPlusTree::bt_key {
private:
	bool pos_inf;
	bool neg_inf;
	size_t len;
	float_key_t val;
	bool is_inf() const {return pos_inf || neg_inf;}
public:
	float_key();
	float_key(const float_key &);
	virtual ~float_key();

	size_t length() const {return len;}
	const void * data() const {return &val;}

	// load from memory, serialzable_ptr->load(ptr_to_position);
	void load(const void *);
	// Used for debug
	std::string to_string() const;

	bool operator<(const float_key &that);
	bool operator==(const float_key &that);

	bool operator<(const comparable &rhs);
	bool operator==(const comparable &rhs);

	void set_inf(int flag);

	bt_key* clone() const {
		return new float_key(*this);
	}
};

#endif /* FLOATKEY_H_ */
