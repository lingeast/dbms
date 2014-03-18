
#include "qe.h"
#include "../bpt/key/intkey.h"
#include "../bpt/key/floatkey.h"
#include "../bpt/key/varcharkey.h"
#include "aggregate.h"
#include <cstring>
#include <climits>
#include <limits>

bool compareData(void* data1,void* data2, CompOp op , AttrType type, int lengthl, int lengthr){
	switch(op){
	case 0: {
			switch(type){
			case 0: return (*(int32_t*)data1 == *(int32_t*)data2);
			case 1: return (*(float*)data1 == *(float*)data2);
			case 2: return (lengthl==lengthr)?(memcmp(data1,data2,lengthl) == 0):0;
			}
			break;
		}
	case 1: {
			switch(type){
			case 0: return (*(int32_t*)data1 < *(int32_t*)data2);
			case 1: return (*(float*)data1 < *(float*)data2);
			case 2: return (memcmp(data1,data2,lengthl) < 0||(memcmp(data1,data2,lengthl) == 0 && lengthl < lengthr));
			}
			break;
		}
	case 2: {
			switch(type){
			case 0: return (*(int32_t*)data1 > *(int32_t*)data2);
			case 1: return (*(float*)data1 > *(float*)data2);
			case 2: return (memcmp(data1,data2,lengthl) > 0||(memcmp(data1,data2,lengthl) == 0 && lengthl > lengthr));
			}
			break;
		}
	case 3: {
			switch(type){
			case 0: return (*(int32_t*)data1 <= *(int32_t*)data2);
			case 1: return (*(float*)data1 <= *(float*)data2);
			case 2: return !(memcmp(data1,data2,lengthl) > 0||(memcmp(data1,data2,lengthl) == 0 && lengthl > lengthr));
			}
			break;
		}
	case 4: {
			switch(type){
			case 0: return (*(int32_t*)data1 >= *(int32_t*)data2);
			case 1: return (*(float*)data1 >= *(float*)data2);
			case 2: return !(memcmp(data1,data2,lengthl) < 0||(memcmp(data1,data2,lengthl) == 0 && lengthl < lengthr));
			}
			break;
		}
	case 5: {
			switch(type){
			case 0: return !(*(int32_t*)data1 == *(int32_t*)data2);
			case 1: return !(*(float*)data1 == *(float*)data2);
			case 2: return (!(memcmp(data1,data2,lengthl) == 0))||(lengthl!=lengthr);
			}
			break;
		}
	case 6: return 1;
	}

	return 0;
}


bool checkCondition(Condition condition, void* data, vector<Attribute> &attrs){
	int offset = 0, attrnumr = 0, attrnuml = 0;
	void *attrdatar,*attrdatal;
	int findAttr = 1 + condition.bRhsIsAttr;
	int lengthl, lengthr = 0;
	for(int i = 0; i < attrs.size(); i++)
		if((condition.lhsAttr).compare(attrs[i].name) == 0){
			switch((attrs[i]).type){
				case 0:{
					lengthl = sizeof(int32_t);
					attrdatal = malloc(sizeof(int32_t));
					memcpy(attrdatal, (char*)data + offset, sizeof(int32_t));
				}
				break;
				case 1:{
					lengthl = sizeof(float);
					attrdatal = malloc(sizeof(float));
					memcpy(attrdatal, (char*)data + offset, sizeof(float));
				}
				break;
				case 2:{
					lengthl = *(int32_t*)((char*)data + offset);
					attrdatal = malloc(lengthl);
					memcpy(attrdatal, (char*)data + offset + sizeof(int32_t), lengthl);
				}
				break;
			}
			findAttr -= 1;
			attrnuml = i;
			if (findAttr == 0) break;
		}else{
			if(condition.bRhsIsAttr && (condition.rhsAttr).compare(attrs[i].name) == 0){
				switch((attrs[i]).type){
					case 0:{
						lengthr = sizeof(int32_t);
						attrdatar = malloc(sizeof(int32_t));
						memcpy(attrdatar, (char*)data + offset, sizeof(int32_t));
					}
					break;
					case 1:{
						lengthr = sizeof(float);
						attrdatar = malloc(sizeof(float));
						memcpy(attrdatar, (char*)data + offset, sizeof(float));
					}
					break;
					case 2:{
						lengthr = *(int32_t*)((char*)data + offset);
						attrdatar = malloc(lengthr);
						memcpy(attrdatar, (char*)data + offset + sizeof(int32_t), lengthr);
					}
					break;
				}
				findAttr -= 1;
				attrnumr = i;
				if (findAttr == 0) break;
			}
			switch((attrs[i]).type){
				case 0: offset += sizeof(int32_t); break;
				case 1: offset += sizeof(float); break;
				case 2: offset += sizeof(int32_t) + *(int32_t*)((char*)data + offset); break;
			}
		}

	if (condition.bRhsIsAttr != true){
		if (condition.rhsValue.type == 2){
			lengthr = *(int32_t*)(condition.rhsValue.data);
			attrdatar = malloc(lengthr);
			memcpy(attrdatar,(char*)condition.rhsValue.data + sizeof(int32_t), lengthr);
		}
		else {
			lengthr = (condition.rhsValue.type==0)? sizeof(int32_t):sizeof(float);
			attrdatar = malloc(lengthr);
			memcpy(attrdatar,condition.rhsValue.data,lengthr);
		}
	}
	return compareData(attrdatal,attrdatar,condition.op,attrs[attrnuml].type,lengthl,lengthr);

}


Filter::Filter(Iterator* input, const Condition &condition):condition(condition), itr(input){
	itr->getAttributes(this->attrs);
	this->attrName = condition.lhsAttr;
}

RC Filter::getNextTuple(void *data){
	bool satisfy = false;
	int ifeof = 0;
	while(satisfy == false&&ifeof != QE_EOF){
		ifeof = itr->getNextTuple(data);
		satisfy = checkCondition(this->condition, data, this->attrs);
	}
	return (ifeof == QE_EOF)?QE_EOF:0;
}

void Filter::getAttributes(vector<Attribute> &attrs) const{
	itr->getAttributes(attrs);
}

// ... the rest of your implementations go here
Project::Project(Iterator *input, const vector<string> &attrNames) : nested(input)
{
	if (nested == NULL) {
		assert(false);
	}
	input->getAttributes(nested_attrs);
	for (int i = 0; i < attrNames.size(); i++) {
		this->attrs.push_back(findAttr(nested_attrs, attrNames[i]));
	}
}

Project::~Project() {
	/*
	 *  Not take care of mem management of input iterator
	if (nested != NULL)
		delete nested;
	*/

}

Attribute Project::findAttr(const vector<Attribute>& attrs, const string& attrName) {
	for (int i = 0; i < attrs.size(); i++) {
		if (attrs[i].name == attrName) {
			return attrs[i];
		}
	}
	Attribute emptyAttr;
	return emptyAttr;
}

void Project::getAttributes(vector<Attribute> &attrs) const {
	attrs = this->attrs;
}

RC Project::getNextTuple(void *data) {
	//int const MAX_VARCHAR_LEN = 100;
	int buffer_len = 0;
	for (int i = 0; i < nested_attrs.size(); i++) {
		buffer_len += nested_attrs[i].length;
	}

	char* buffer = new char[buffer_len];
	if (nested->getNextTuple((void*) buffer) == QE_EOF) // read nested output in to buffer
		return QE_EOF; // end of file

	// project some fields from buffer to data
	char* off_data = (char*) data;
	for (int i = 0; i < attrs.size(); i++) {
		char* off_buf = buffer;
		for (int j = 0; j < nested_attrs.size(); j++) {
			Attribute& nest_attr= nested_attrs[j];
			int len = RawDataUtil::fieldLen(off_buf, nest_attr);
			if (attrs[i].name != nest_attr.name) {
				off_buf += len;
			} else {	// reach wanted field
				memcpy(off_data, off_buf, len);
				off_data += len;
				off_buf += len;
				break; // should only have 1 match
			}
		}
	}

	delete buffer;
	return 0;
}

/*
int fieldLen(void* data, const Attribute attr) {
	switch(attr.type) {
	case TypeInt:
		return attr.length;
	case TypeReal:
		return attr.length;
	case TypeVarChar:
		return *(uint32_t*)data + sizeof(uint32_t);
	}
}*/

int RawDataUtil::fieldLen(void* data, const Attribute& attr) {
	//cout << "In RDU fieldLen" << endl;
	switch(attr.type) {
	case TypeInt:
		return attr.length;
	case TypeReal:
		return attr.length;
	case TypeVarChar:
		return *(uint32_t*)data + sizeof(uint32_t);
	default:
		return -1;	// indicate unsuccesful call
	}
}

int RawDataUtil::recordLen(void* data, const vector<Attribute>& attrs) {
	char* off_data = (char*) data;
	int len = 0;
	for(int i = 0; i < attrs.size(); i++) {
		len += fieldLen(off_data + len, attrs[i]);
	}
	return len;
}

int RawDataUtil::recordMaxLen(const vector<Attribute>& attrs) {
	int max_len = 0;
	for (int i = 0; i < attrs.size(); i++) {
		max_len += attrs[i].length;
	}
	return max_len;
}

int RawDataUtil::getAttrOff(const void* record, const vector<Attribute> attrs, const string& attrName) {
	int len = 0;
	char* data = (char*) record;

	for (int i = 0; i < attrs.size(); i++) {
		if(attrs[i].name == attrName) return len;
		else {
			switch(attrs[i].type) {
			case TypeInt:
				len += attrs[i].length; break;
			case TypeReal:
				len += attrs[i].length; break;
			case TypeVarChar:
				len += *(uint32_t*)(data + len) + sizeof(uint32_t);
			}
		}
	}
	return -1; //indicating match failure
}
NLJoin::NLJoin(Iterator *leftIn,                             // Iterator of input R
               TableScan *rightIn,                           // TableScan Iterator of input S
               const Condition &condition,                   // Join condition
               const unsigned numPages                       // Number of pages can be used to do join (decided by the optimizer)
        ): LItr(leftIn),RItr(rightIn),condition(condition),numPages(numPages),Ldata(NULL),Lattr(NULL),Ldatalen(0),Lattrlen(0),type(TypeInt),round(0),count(0){
	LItr->getAttributes(this->LattrList);
	RItr->getAttributes(this->RattrList);
};

int NLJoin::getAttr(void* data, string attrname, vector<Attribute> list,int &attrlen, int &datalen){
	int offset = 0;
	int attrpos = 0;
	for(int i = 0; i < list.size(); i++){
		if(attrname.compare(list[i].name) == 0){
			if(list[i].type == 2){
				attrlen = *(int32_t*)((char*)(data) + offset);
				attrpos = offset;
				this->type = list[i].type;
			}else{
				attrlen = list[i].type == 0? sizeof(int32_t):sizeof(float);
				attrpos = offset;
				this->type = list[i].type;
			}
		}
		if(list[i].type == 2){
			offset += *(int32_t*)((char*)(data) + offset);
			offset += sizeof(int32_t);
		}
		else offset += (list[i].type == 0)? sizeof(int32_t):sizeof(float);
	}
	datalen = offset;
	return attrpos;
}

RC NLJoin::getNextTuple(void *data){
	void *Rattr,*Rdata;
	int Rattrlen = 0,Rdatalen = 0,attrpos = 0;
	if(Ldata == NULL){
		if(LItr->getNextTuple(data) == QE_EOF)
			return QE_EOF;
		attrpos = getAttr(data,condition.lhsAttr,LattrList,Lattrlen,Ldatalen);
		Lattr = malloc(Lattrlen);
		memcpy(Lattr,(char*)data + attrpos, Lattrlen);
		Ldata = malloc(Ldatalen);
		memcpy(Ldata,data,Ldatalen);
	}
	if(round == 0){
		int j = 1;
	}
	int flag = 0;
	do{
		if (flag!=0){
			attrpos = getAttr(data,condition.lhsAttr,LattrList,Lattrlen,Ldatalen);
			Lattr = malloc(Lattrlen);
			memcpy(Lattr,(char*)data + attrpos, Lattrlen);
			Ldata = malloc(Ldatalen);
			memcpy(Ldata,data,Ldatalen);
		}
		while(RItr->getNextTuple(data)!=QE_EOF){
			attrpos = getAttr(data,condition.rhsAttr,RattrList,Rattrlen,Rdatalen);
			Rattr = malloc(Rattrlen);
			memcpy(Rattr,(char*)data + attrpos, Rattrlen);
			Rdata = malloc(Rdatalen);
			memcpy(Rdata,data,Rdatalen);
			if(compareData(Lattr, Rattr, condition.op, this->type, Lattrlen, Rattrlen)==true){
				memcpy(data,Ldata,Ldatalen);
				memcpy((char*)data + Ldatalen,Rdata,Rdatalen);
				count ++;
				return 0;
			}
			free(Rdata);
			free(Rattr);
		}
		free(Ldata);
		free(Lattr);
		Ldatalen = 0;
		Lattrlen = 0;
		flag = 1;
		//cout<<"round: "<<round<<" find "<<count<<endl;
		count = 0;
		round += 1;
		RItr->setIterator();
	}while(LItr->getNextTuple(data)!=QE_EOF);
	return QE_EOF;
}

void NLJoin::getAttributes(vector<Attribute> &attrs) const{
	for(int i = 0; i < LattrList.size(); i++){
		attrs.push_back(LattrList[i]);
	}
	for(int i = 0; i < RattrList.size(); i++){
			attrs.push_back(RattrList[i]);
	}
}

INLJoin::INLJoin(Iterator *leftIn,                               // Iterator of input R
        IndexScan *rightIn,                             // IndexScan Iterator of input S
        const Condition &condition,                     // Join condition
        const unsigned numPages                         // Number of pages can be used to do join (decided by the optimizer)
) : lItr(leftIn), is(rightIn),
		cond(condition), pageLim(numPages),
		jBuf(NULL), lBuf(NULL), rBuf(NULL), reachEnd(false) {
	lItr->getAttributes(lAttrs);
	is->getAttributes(rAttrs);
	this->jAttrs = lAttrs;
	this->jAttrs.insert(jAttrs.end(), rAttrs.begin(), rAttrs.end());
	//cout << "jBuffer Size = " << RawDataUtil::recordMaxLen(jAttrs) << endl;
	jBuf = new char[RawDataUtil::recordMaxLen(jAttrs)];
	//cout << "lBuffer Size = " << RawDataUtil::recordMaxLen(lAttrs) << endl;
	lBuf = new char[RawDataUtil::recordMaxLen(lAttrs)];
	//cout << "lBuffer Size = " << RawDataUtil::recordMaxLen(rAttrs) << endl;
	rBuf = new char[RawDataUtil::recordMaxLen(rAttrs)];
	if (leftIn->getNextTuple(lBuf) == -1) {	// leftItr is completely empty
		reachEnd = true;
	}

	if (is->getNextTuple(rBuf) == -1) {	// rightIndexScan is completely empty
		reachEnd = true;
	}
}
INLJoin::~INLJoin() {
	if (lBuf != NULL) delete[] lBuf;
	if (jBuf != NULL) delete[] jBuf;
	if (rBuf != NULL) delete[] rBuf;
}
void INLJoin::getAttributes(vector<Attribute> &attrs) const {
	attrs = jAttrs;
}

RC INLJoin::getNextTuple(void* data) {
	if (this->reachEnd) return QE_EOF;
	while(true) {
		if (this->checkCondition(lBuf, rBuf, lAttrs, rAttrs, cond)) {
			// do concatenation  write
			char* cur = (char*) data;
			int len = RawDataUtil::recordLen(lBuf, lAttrs);
			memcpy(cur, lBuf, len);
			cur += len;
			len = RawDataUtil::recordLen(rBuf, rAttrs);
			memcpy(cur, rBuf, len);

			increPair(); //prepare for next read
			return 0;
		} else {
			if (!increPair()) {
				return QE_EOF;
			}
		}
	}
	assert(false); // should not reach here
	return -1;
}

bool INLJoin::increPair() {
	if (is->getNextTuple(rBuf) == -1) {	// read next index into rBuf
		// indexScan reach End
		if (lItr->getNextTuple(lBuf) == -1) {	// read next itr into lBuf
			reachEnd = true;
			return false;
		} else { // read next itr successfully
			is->setIterator(NULL, NULL, false, false);
			if (is->getNextTuple(rBuf) == -1) {
				assert(false); // should not happen because of reachEnd field protection
				return false;
			} else {
				return true;
			}
		}
	} else {
		// read next index successfully
		return true;
	}
}
/*
int getAttrOff(const void* record, const vector<Attribute> attrs, const string& attrName) {
	int len = 0;
	char* data = (char*) record;

	for (int i = 0; i < attrs.size(); i++) {
		if(attrs[i].name == attrName) return len;
		else {
			switch(attrs[i].type) {
			case TypeInt:
				len += attrs[i].length; break;
			case TypeReal:
				len += attrs[i].length; break;
			case TypeVarChar:
				len += *(uint32_t*)(data + len) + sizeof(uint32_t);
			}
		}
	}
	return -1; //indicating match failure
}
*/

bool INLJoin::checkCondition(void* lData, void* rData,
		vector<Attribute>& lA, vector<Attribute>& rA,
		const Condition& cond) {
	// get corresponding field offset of lAttr
	int offL = RawDataUtil::getAttrOff(lData, lA, cond.lhsAttr);
	if (offL < 0) return false; // fail to find left field

	//TODO remove test code
	/*
	bt_key* testk = new float_key();
	testk->load((char*)rData + 4);
	cout << "Index Scan: 1 = "<< testk->to_string() << endl;
	*/

	bt_key *lhs = NULL;
	bt_key *rhs = NULL;
	for (int i = 0; i < lA.size(); i++) {
		if (cond.lhsAttr == lA[i].name) {
			switch(lA[i].type) {
			case TypeInt:
				lhs = new int_key(); rhs = new int_key(); break;
			case TypeReal:
				lhs = new float_key(); rhs = new float_key(); break;
			case TypeVarChar:
				lhs = new varchar_key(); rhs = new varchar_key(); break;
			}
		}
	}

	lhs->load((char*)lData + offL);
	if (cond.bRhsIsAttr) {
		rhs->load(((char*)rData) + RawDataUtil::getAttrOff(rData, rA, cond.rhsAttr));
		//cout << "Offset " << offL << "=" << getAttrOff(rData, rA, cond.rhsAttr);
	} else {
		rhs->load(cond.rhsValue.data);
	}

	//cout << "Compare "<<":"<< lhs->to_string() << ", " << rhs->to_string() << endl;
	switch(cond.op) {
	case EQ_OP: return (*lhs == *rhs);
	case LT_OP: return (*lhs < *rhs);      // <
	case GT_OP: return (*rhs < *lhs);      // >
	case LE_OP: return (*lhs < *rhs || *lhs == *rhs);   // <=
	case GE_OP: return (!(*lhs < *rhs));     // >=
	case NE_OP: return !((*lhs == *rhs));      // !=
	case NO_OP: return true;       // no condition
	default: assert(false); return false;
	}
}

Aggregate::Aggregate(Iterator* input, Attribute aggAttr, AggregateOp op):
		input(input), attr(aggAttr), op(op), finished(false) {
	input->getAttributes(iAttrs);
}

RC Aggregate::getNextTuple(void* data) {
	if (finished) return QE_EOF;
	// do aggregation and write back
	char* buffer = new char[RawDataUtil::recordMaxLen(iAttrs)];
	if (attr.type == TypeInt) {
		float total;
		int32_t one;
		assert(sizeof(float) == attr.length);

		int cnt = 0;
		switch(this->op) {
		case (MIN) : total = INT_MAX * 1.0 ; break;
		case (MAX) : total = INT_MIN * 1.0 ; break;
		default: total = 0; break;
		}
		while(input->getNextTuple(buffer) != QE_EOF) {
			memcpy(&one,
					buffer + RawDataUtil::getAttrOff(buffer, iAttrs, attr.name),
					sizeof(one));
			++cnt;
			switch(this->op) {
			case (MIN) : total = mintem(total, one); break;
			case (MAX) : total = maxtem(total, one); break;
			case (COUNT): break;
			default: total = sumtem(total, one); break;
			}
		}

		switch(op) {
		case(COUNT): total = cnt; break;
		case(AVG): total = total/ cnt; break;
		default: break;
		}
		if (op == AVG) {
			memcpy(data, &total, sizeof(total));
		} else {
			int32_t tmp = total;
			memcpy(data, &tmp, sizeof(total));
		}
	} else if(attr.type == TypeReal) {
		float total;
		float one;
		assert(sizeof(float) == attr.length);

		int cnt = 0;
		switch(this->op) {
		case (MIN) : total = INT_MIN; break;
		case (MAX) : total = INT_MAX; break;
		default: total = 0; break;
		}
		while(input->getNextTuple(&one) != QE_EOF) {
			memcpy(&one,
					buffer + RawDataUtil::getAttrOff(buffer, iAttrs, attr.name),
					sizeof(one));
			++cnt;
			switch(this->op) {
			case (MIN) : total = mintem(total, one); break;
			case (MAX) : total = maxtem(total, one); break;
			case (COUNT): break;
			default: total = sumtem(total, one); break;
			}
		}

		switch(op) {
		case(COUNT): total = cnt; break;
		case(AVG): total = total/ cnt; break;
		default: break;
		}

		if (op == COUNT) {
			int32_t tmp = total;
			memcpy(data, &tmp, sizeof(tmp));
		} else {
			memcpy(data, &total, sizeof(total));
		}
	} else {
		assert(false);
	}

	// set finished flag
	finished = true;
	return 0;
}

void Aggregate::getAttributes(vector<Attribute> &attrs) const {
	attrs = this->iAttrs;
}

