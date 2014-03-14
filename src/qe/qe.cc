
#include "qe.h"

Filter::Filter(Iterator* input, const Condition &condition) {
}

// ... the rest of your implementations go here
Project::Project(Iterator *input, const vector<string> &attrNames) : nested(input),
		attrs(attrNames)
{
	if (nested == NULL) {
		assert(false);
		~Project();
	}
	input->getAttributes(nested_attrs);
}

Project::~Project() {
	delete nested;
}

void Project::getAttributes(vector<Attribute> &attrs) const {
	attrs = this->attrs;
}

RC Project::getNextTuple(void *data) {
	int const MAX_VARCHAR_LEN = 100;
	int buffer_len = 0;
	for (int i = 0; i < nested_attrs.size(); i++) {
		buffer_len += nested_attrs[i].length;
	}

	char buffer = new char[buffer_len];
	if (nested->getNextTuple((void*) buffer) == QE_EOF) // read nested output in to buffer
		return QE_EOF; // end of file

	char* cur = data;



	delete buffer;
}
