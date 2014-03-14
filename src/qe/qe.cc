
#include "qe.h"

Filter::Filter(Iterator* input, const Condition &condition) {
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
			int len = fieldLen(off_buf, nest_attr);
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


int Project::fieldLen(void* data, Attribute attr) {
	switch(attr.type) {
	case TypeInt:
		return attr.length;
	case TypeReal:
		return attr.length;
	case TypeVarChar:
		return *(uint32_t*)data + sizeof(uint32_t);
	}
}
