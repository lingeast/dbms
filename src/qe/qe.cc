
#include "qe.h"

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
	while(satisfy == false&&ifeof == 0){
		ifeof = itr->getNextTuple(data);
		satisfy = checkCondition(this->condition, data, this->attrs);
	}
	return (ifeof == -1)?QE_EOF:0;
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
NLJoin::NLJoin(Iterator *leftIn,                             // Iterator of input R
               TableScan *rightIn,                           // TableScan Iterator of input S
               const Condition &condition,                   // Join condition
               const unsigned numPages                       // Number of pages can be used to do join (decided by the optimizer)
        ): LItr(leftIn),RItr(rightIn),condition(condition),numPages(numPages),Ldata(NULL),Lattr(NULL),Ldatalen(0),Lattrlen(0),type(TypeInt){
	LItr->getAttributes(this->LattrList);
	RItr->getAttributes(this->RattrList);
};

void NLJoin::getAttr(void* data, string attrname, void* attr, vector<Attribute> list,int &attrlen, int &datalen){
	int offset = 0;
	for(int i = 0; i < list.size(); i++){
		if(attrname.compare(list[i].name) == 0){
			if(list[i].type == 2){
				attrlen = *(int32_t*)((char*)(data) + offset);
				attr = malloc(attrlen);
				memcpy(attr,(char*)data + offset + sizeof(int32_t),attrlen);
				this->type = list[i].type;
			}else{
				attrlen = list[i].type == 0? sizeof(int32_t):sizeof(float);
				memcpy(attr,(char*)data + offset, attrlen);
				this->type = list[i].type;
			}
		}
		if(list[i].type == 2)
			offset += *(int32_t*)((char*)(data) + offset);
		else offset += (list[i].type == 0)? sizeof(int32_t):sizeof(float);
	}
	datalen = offset;
}

RC NLJoin::getNextTuple(void *data){
	void *Rattr,*Rdata;
	int Rattrlen = 0,Rdatalen = 0;
	if(Ldata == NULL){
		if(LItr->getNextTuple(data) == QE_EOF)
			return QE_EOF;
		getAttr(data,condition.lhsAttr,Lattr,LattrList,Lattrlen,Ldatalen);
		Ldata = malloc(Ldatalen);
		memcpy(Ldata,data,Ldatalen);
	}
	int round = 0;
	do{
		if (round!=0){
			getAttr(data,condition.lhsAttr,Lattr,LattrList,Lattrlen,Ldatalen);
			Ldata = malloc(Ldatalen);
			memcpy(Ldata,data,Ldatalen);
		}
		while(RItr->getNextTuple(data)!=QE_EOF){
			getAttr(data,condition.rhsAttr,Rattr,RattrList,Rattrlen,Rdatalen);
			Rdata = malloc(Rdatalen);
			memcpy(Rdata,data,Rdatalen);
			if(compareData(Lattr, Rattr, condition.op, this->type, Lattrlen, Rattrlen)==true){
				memcpy(data,Ldata,Ldatalen);
				memcpy((char*)data + Ldatalen,Rdata,Rdatalen);
				return 0;
			}
			free(Rdata);
			free(Rattr);
		}
		free(Ldata);
		free(Lattr);
		Ldatalen = 0;
		Lattrlen = 0;
		round = 1;
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
