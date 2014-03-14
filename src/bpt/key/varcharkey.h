/*
 * varcharkey.h
 *
 *  Created on: Feb 27, 2014
 *      Author: yidong
 */

#ifndef VARCHARKEY_H_
#define VARCHARKEY_H_

#include "btkey.h"
#include <string>

using std::string;
using namespace BPlusTree;
/*
 * raw_data structure:
 * | 4-byte len | string data |
 */
class varchar_key : public bt_key {
private:
	bool pos_inf;
	bool neg_inf;
	char* raw_data;
	int len;
	string str;
	bool is_inf() const {return pos_inf || neg_inf;}
public:
	size_t length() const;
	const void* data() const ;

	// load from memory, serialzable_ptr->load(ptr_to_position);
	void load(const void *);
	// Used for debug
	std::string to_string() const;

	varchar_key(); // default constructor
	varchar_key(const varchar_key&);

	bool operator<(const varchar_key &that);
	bool operator==(const varchar_key &that);

	bool operator<(const comparable &rhs);
	bool operator==(const comparable &rhs);

	void set_inf(int flag);
	~varchar_key();

	bt_key* clone() const {
		return new varchar_key(*this);
	}
};

#endif /* VARCHARKEY_H_ */
