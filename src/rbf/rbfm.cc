
#include "rbfm.h"
#include <assert.h>
#include <cstdlib>
#include <cstring>


RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
}

RecordBasedFileManager::~RecordBasedFileManager()
{
	if (_rbf_manager)
		delete _rbf_manager;
	_rbf_manager = NULL;
}

RC RecordBasedFileManager::createFile(const string &fileName) {
	return PagedFileManager::instance()->createFile(fileName.c_str());
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
	return PagedFileManager::instance()->destroyFile(fileName.c_str());
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
	//cout << "Open a file" << endl;
	return PagedFileManager::instance()->openFile(fileName.c_str(),fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
	//cout << "Close a file" << endl;
    return PagedFileManager::instance()->closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {

	int length = 0;
	void* newrecord = buildRecord(recordDescriptor, data, &length);
	// find page to insert the record
	int newremain = 0;


	int ret = fileHandle.findfreePage(length, &newremain);
	if (ret == -1) {free(newrecord);return -1;};
	rid.pageNum  = ret;
	/*
	if (fileHandle.findfreePage(length, &newremain) == -1) {
		assert(false);
		free(newrecord);
		return -1;
	}
	*/

	PageHandle ph(rid.pageNum, fileHandle);
	// insertRecord to target page
	rid.slotNum = ph.insertRecord(newrecord, length, &newremain,0);
	// if need reorganize
	if (rid.slotNum == -1) {
		ph.reorganizePage();
		rid.slotNum = ph.insertRecord(newrecord, length, &newremain,0);
	}
	free(newrecord);	// Add free() to free dynamic allocated memory, LYD JAN 24 2014
	try{
		// write back page and page remaining info
		fileHandle.writePage(rid.pageNum, ph.dataBlock());
		fileHandle.setNewremain(rid.pageNum,newremain);
	}catch(const std::exception &e){
		std::cout<< e.what() << std::endl;
		return -1;
	}
	return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	PageHandle ph(rid.pageNum,fileHandle);
	int length = 0;
	for (unsigned int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				length += recordDescriptor[i].length;
				break;
			case 1:
				length += recordDescriptor[i].length;
				break;
			case 2:
				length += recordDescriptor[i].length;
				break;
		}
	}
	// find the approximate length of the record we stored
	length += sizeof(int16_t) * (recordDescriptor.size() + 1);
	void* storedRecord = malloc(length);
	RID exactrid = rid;
	do{
		// find the exact the record place
		if (length == 0){
			exactrid.pageNum = *((int32_t*) storedRecord);
			exactrid.slotNum = *((int16_t*) storedRecord + sizeof(int32_t)/sizeof(int16_t));
			ph.loadPage(exactrid.pageNum,fileHandle);
		}
		length = ph.readRecord(exactrid.slotNum, storedRecord);
	}while(length == 0);
	if (length < 0){
		free(storedRecord);
		return -1;
	}
	revertRecord(recordDescriptor,data,storedRecord);
	free(storedRecord);

    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	int dataoffset = 0;
	cout<<"============Print record============"<<endl;
	for (unsigned int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				cout<<"Recordfield "<<i<<endl;
				cout<<"FieldType: int"<<endl;
				cout<<"FieldContent: "<<*(int*)((char*)data + dataoffset)<<endl;
				dataoffset += sizeof(int32_t);
				break;
			case 1:
				cout<<"Recordfield: "<<i<<endl;
				cout<<"FieldType: real"<<endl;
				cout<<"FieldContent: "<<*(float*)((char*)data + dataoffset)<<endl;
				dataoffset += sizeof(float);
				break;
			case 2:
				cout<<"Recordfield "<<i<<endl;
				cout<<"FieldType: string"<<endl;
				int* stringlen = new int;
				memcpy(stringlen,(char*)data + dataoffset, sizeof(int32_t));
				char* str = (char*)(malloc(sizeof(char)*(*stringlen + 1)));
				memcpy(str,(char*)data + dataoffset + sizeof(int32_t),*stringlen);
				dataoffset += *stringlen + sizeof(int32_t);
				cout<<"FieldContent: "<<str<<endl;
				free(stringlen);
				// TODO Free
				free(str);
				break;
		}
		cout<<endl;
	}
	return -1;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){
	PageHandle ph(rid.pageNum,fileHandle);
	RID prevrid = rid;
	RID exactrid = rid;
	RC result;
	int newremain = 0;
	result = ph.deleteRecord(&exactrid.slotNum, &exactrid.pageNum, &newremain);
	// if migrate to other page
	while(result == 1){
		try{
				// write back page and page remaining info
				fileHandle.writePage(prevrid.pageNum, ph.dataBlock());
				//fileHandle.setNewremain(rid.pageNum,newremain);
			}catch(const std::exception &e){
				std::cout<< e.what() << std::endl;
				return -1;
		}

		prevrid = exactrid;
		// delete the migrate record recursively
		ph.loadPage(exactrid.pageNum,fileHandle);
		result = ph.deleteRecord(&exactrid.slotNum, &exactrid.pageNum, &newremain);
	}
	try{
		// write back page and page remaining info
		fileHandle.writePage(exactrid.pageNum, ph.dataBlock());
		fileHandle.setNewremain(exactrid.pageNum,newremain);
	}catch(const std::exception &e){
		std::cout<< e.what() << std::endl;
		return -1;
	}
	return result;
}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){
	int length = 0, newremain = 0;
	RC result= 0;
	void* newrecord = buildRecord(recordDescriptor, data, &length);
	PageHandle ph(rid.pageNum,fileHandle);
	RID exactrid = rid;
	result = ph.updateRecord(exactrid.slotNum,newrecord,length,&newremain,&exactrid.pageNum,&exactrid.slotNum);
	while(result ==1){
		//find the exact position of the record
		ph.loadPage(exactrid.pageNum,fileHandle);
		result = ph.updateRecord(exactrid.slotNum,newrecord,length,&newremain,&exactrid.pageNum,&exactrid.slotNum);
	}
	if (result == 0){
		// update done
		try{
			// write back page and page remaining info
			fileHandle.writePage(exactrid.pageNum, ph.dataBlock());
			fileHandle.setNewremain(exactrid.pageNum,newremain);
		}catch(const std::exception &e){
			std::cout<< e.what() << std::endl;
			return -1;
		}
		return 0;
	}
	if (result == 2){
		// need migrate
		RID newrid;
		newrid.pageNum = fileHandle.findfreePage(length, &newremain);
		if (newrid.pageNum == -1) {free(newrecord);return -1;};
		PageHandle Migph(newrid.pageNum, fileHandle);
		// insertRecord to target page
		newrid.slotNum = Migph.insertRecord(newrecord, length, &newremain,1);
		// if need reorganize
		if (newrid.slotNum == -1) {
			//assert(false);
			Migph.reorganizePage();
			newrid.slotNum = Migph.insertRecord(newrecord, length, &newremain,1);
		}
		free(newrecord);	// Add free() to free dynamic allocated memory, LYD JAN 24 2014
		try{
			// write back page and page remaining info
			fileHandle.writePage(newrid.pageNum, Migph.dataBlock());
			fileHandle.setNewremain(newrid.pageNum,newremain);
		}catch(const std::exception &e){
			std::cout<< e.what() << std::endl;
			return -1;
		}
		ph.setMigrate(exactrid.slotNum, newrid.pageNum,newrid.slotNum,&newremain);
		try{
			// write back page and page remaining info
			fileHandle.writePage(exactrid.pageNum, ph.dataBlock());
			fileHandle.setNewremain(exactrid.pageNum,newremain);
		}catch(const std::exception &e){
			std::cout<< e.what() << std::endl;
			return -1;
		}
		return 0;
	}
	if (result == 3){
		//need reorganize
		//cout << "Page:" << exactrid.pageNum << "Slot: " << exactrid.slotNum << " cause reorganized "<<endl;
		ph.reorganizePage();
		ph.updateRecord(exactrid.slotNum,newrecord,length,&newremain,&exactrid.pageNum,&exactrid.slotNum);
		try{
			// write back page and page remaining info
			fileHandle.writePage(exactrid.pageNum, ph.dataBlock());
			fileHandle.setNewremain(exactrid.pageNum,newremain);
		}catch(const std::exception &e){
			std::cout<< e.what() << std::endl;
			return -1;
		}
		return 0;
	}

	free(newrecord);
	return -1;
}
RC RecordBasedFileManager::readAttribute(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, const string attributeName, void *data){
	// TODO change malloc
	char record[PAGE_SIZE];
	int ret = 0;
	ret = this->readRecord(fileHandle,recordDescriptor,rid,record);
	int i = 0;
	for (i = 0 ; i < recordDescriptor.size() ; i++ ){
		if ((recordDescriptor[i]).name == attributeName){
			break;
		}
	}
	int offset = 0,length = 0;
	if (i == 0) {
		offset = (*((int16_t*)record) + 1) * sizeof(int16_t);
		length = *((int16_t*)((char*)record + sizeof(int16_t))) - offset;
	}
	else {
		offset = *((int16_t*)((char*)record + sizeof(int16_t) * i));
		length = *((int16_t*)((char*)record + sizeof(int16_t) * (i + 1)));
	}
	memcpy(data,(char*)record + offset, length);
	return ret;
}

RC RecordBasedFileManager::reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber){
	PageHandle ph(pageNumber,fileHandle);
	int32_t newremain = 0;
	newremain = ph.reorganizePage();
	//cout<<"test1"<<endl;
	try{
		// write back page and page remaining info
			fileHandle.writePage(pageNumber, ph.dataBlock());
	}catch(const std::exception &e){
		std::cout<< e.what() << std::endl;
		return -1;
	}
	return 0;
}


void* RecordBasedFileManager::buildRecord(const vector<Attribute> &recordDescriptor,const void *data,int* recordlenth)
{
	int length = 0;
	int dataoffset = 0;
	// calculate the length of the record we will insert
	for (unsigned int i = 0 ; i < recordDescriptor.size() ; i++){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				length += recordDescriptor[i].length;
				dataoffset += recordDescriptor[i].length;
				break;
			case 1:
				length += recordDescriptor[i].length;
				dataoffset += recordDescriptor[i].length;
				break;
			case 2:
				int* stringlen = new int;
				memcpy(stringlen,(char*)data + dataoffset, sizeof(int32_t));
				length += *stringlen;
				dataoffset += *stringlen + sizeof(int32_t);
				free(stringlen);
				break;
		}
	}
	void* newRecord = malloc(length + sizeof(int16_t) + sizeof(int16_t) * recordDescriptor.size());
    // First int16_t integer in record represents the fields number;
	*((int16_t*)(newRecord)) = recordDescriptor.size();
    int16_t offset = sizeof(int16_t) + sizeof(int16_t)*recordDescriptor.size();
    dataoffset = 0;
    // change the format of the data to the record
	for (unsigned int i = 0 ; i < recordDescriptor.size() ; i++)
	{
		switch((recordDescriptor[i]).type)
		{
			case 0:
				memcpy((char*)(newRecord)+offset, (char*)(data) + dataoffset, recordDescriptor[i].length);
				offset += sizeof(recordDescriptor[i].length);
				dataoffset += sizeof(recordDescriptor[i].length);
				break;
			case 1:
				memcpy((char*)(newRecord)+offset, (char*)(data) + dataoffset, recordDescriptor[i].length);
				offset += sizeof(recordDescriptor[i].length);
				dataoffset += sizeof(recordDescriptor[i].length);
				break;
			case 2:
				int *stringlen = new int;
				memcpy(stringlen, (char*)(data) + dataoffset, sizeof(int32_t));
				memcpy((char*)(newRecord)+offset, (char*)(data) + dataoffset + sizeof(int32_t), *stringlen);
				offset += *stringlen;
				dataoffset += sizeof(int32_t) + *stringlen;
				free(stringlen);
				break;
		}
		// add an int16_t integer point to the tail byte of each field.
		*((int16_t*)(newRecord) + i + 1) = offset;
	}
	*recordlenth = offset;
	return newRecord;
}

void RecordBasedFileManager::revertRecord(const vector<Attribute> &recordDescriptor, void *data, void* inputdata){
	int offset = 0;
	int recordoffset = sizeof(int16_t) * (*(int16_t*)inputdata + 1);
	for (unsigned int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				memcpy((char*)data + offset,(char*)inputdata + recordoffset, recordDescriptor[i].length);
				offset += recordDescriptor[i].length;
				recordoffset += recordDescriptor[i].length;
				break;
			case 1:
				memcpy((char*)data + offset,(char*)inputdata + recordoffset, recordDescriptor[i].length);
				offset += recordDescriptor[i].length;
				recordoffset += recordDescriptor[i].length;
				break;
			case 2:
				int strlen = 0;
				if (i == 0) strlen = *((int16_t*)inputdata + (i+1)) - sizeof(int16_t) * (*(int16_t*)inputdata + 1);
				else strlen = *((int16_t*)inputdata + (i+1)) - *((int16_t*)inputdata + i);
				memcpy((char*)data + offset, &strlen, sizeof(int32_t));
				offset += sizeof(int32_t);
				memcpy((char*)data + offset, (char*)inputdata + recordoffset, strlen);
				offset += strlen;
				recordoffset += strlen;
				break;
		}
	}
}

RC RecordBasedFileManager::scan(FileHandle &fileHandle,
      const vector<Attribute> &recordDescriptor,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RBFM_ScanIterator &rbfm_ScanIterator){

	rbfm_ScanIterator.setIterator(fileHandle,recordDescriptor,conditionAttribute,compOp,value,attributeNames,this);
	return -1;
}

RBFM_ScanIterator::RBFM_ScanIterator(){
	this->attributenum = 0;
	this->compOp = NO_OP;
	this->type = 0;
	this->rbfm = NULL;
	this->value = NULL;
}

RBFM_ScanIterator::~RBFM_ScanIterator(){
	/*
	if (value) {
		cout << "Forget to call close?" << endl;
		delete value;
		rbfm->closeFile(fileHandle);
	}
	*/
}

RC RBFM_ScanIterator::close() {
	if (value != NULL) free(value);
	return rbfm->closeFile(fileHandle);
}

void RBFM_ScanIterator::setIterator(FileHandle &fileHandle,
	      const vector<Attribute> &recordDescriptor,
	      const string &conditionAttribute,
	      const CompOp compOp,                  // comparision type such as "<" and "="
	      const void *val,                    // used in the comparison
	      const vector<string> &attributeNames, // a list of projected attributes
	      RecordBasedFileManager *rbfm)
{
	this->fileHandle = fileHandle; 			//deep copy
	this->recordDescriptor = recordDescriptor;		//deep copy
	this->conditionAttribute = conditionAttribute;	// deep copy
	this->compOp = compOp;					//deep copy
	//this->value = value;					// shallow copy
	this->attributeNames = attributeNames;	// deep copy
	this->ph.loadPage(0,fileHandle);		// deep read
	this->currentRID.pageNum = 0;
	this->currentRID.slotNum = 0;
	this->rbfm = rbfm;					// shallow copy singleton class ptr
	for (int j = 0 ; j < attributeNames.size() ; j++ ){
		for (int i = 0 ; i < recordDescriptor.size() ; i++ ){
			if (attributeNames[j].compare((recordDescriptor[i]).name) == 0){
				//cout << attributeNames[j] << "(" << j << ")" << endl;
				this->attriID.push_back(i);
				break;
			}
		}
	}

	//TODO remove test code
	//for (int i = 0; i < attriID.size(); i++) {
		//cout << "attriID: " << attriID[i] << endl;
	//}
	for (int i = 0 ; i < recordDescriptor.size() ; i++ ){
		if ((recordDescriptor[i]).name == conditionAttribute){
			attributenum = i;
			type = (recordDescriptor[i]).type;
			break;
		}
	}

	if (val != NULL) {
		//cout << "Begin to deep copy value in RBFM_ITR" << endl;
		size_t val_len = 0;
		switch(type) {
		case TypeInt:
			val_len = sizeof(uint32_t);
			break;
		case TypeReal:
			val_len = sizeof(uint32_t);
			break;
		case TypeVarChar:
			val_len = *(uint32_t*)val + sizeof(uint32_t);
			break;
		default:
			cout << "Can't be here" << endl;
			assert(false);
		}
		this->value = malloc(val_len);
		memcpy(this->value, val, val_len);
	}


}

RC RBFM_ScanIterator::compareRecord(void* data1, const void *data2, int length){
	if (this->type == 2)
	{
		length = *(int32_t*)data2;
		// TODO test code
		//cout << "strlen: " << length << endl;
		//char* cur = (char*)data2;
		//cur += sizeof(int32_t);
		//for (int i = 0; i < length; i++) {
		//		cout << *(cur + i) << " == " << *((char*)data1 + i) << endl;
		//}
		// test code end

	}


	switch(this->compOp){
	case 0: {
			switch(this->type){
			case 0: return (*(int32_t*)data1 == *(int32_t*)data2);
			case 1: return (*(float*)data1 == *(float*)data2);
			case 2: return (memcmp(data1,(char*)data2 + sizeof(int32_t),length) == 0);
			}
			break;
		}
	case 1: {
			switch(this->type){
			case 0: return (*(int32_t*)data1 < *(int32_t*)data2);
			case 1: return (*(float*)data1 < *(float*)data2);
			case 2: return (memcmp(data1,(char*)data2 + sizeof(int32_t),length) < 0);
			}
			break;
		}
	case 2: {
			switch(this->type){
			case 0: return (*(int32_t*)data1 > *(int32_t*)data2);
			case 1: return (*(float*)data1 > *(float*)data2);
			case 2: return (memcmp(data1,(char*)data2 + sizeof(int32_t),length) > 0);
			}
			break;
		}
	case 3: {
			switch(this->type){
			case 0: return (*(int32_t*)data1 <= *(int32_t*)data2);
			case 1: return (*(float*)data1 <= *(float*)data2);
			case 2: return (memcmp(data1,(char*)data2 + sizeof(int32_t),length) <= 0);
			}
			break;
		}
	case 4: {
			switch(this->type){
			case 0: return (*(int32_t*)data1 >= *(int32_t*)data2);
			case 1: return (*(float*)data1 >= *(float*)data2);
			case 2: return (memcmp(data1,(char*)data2 + sizeof(int32_t),length) >= 0);
			}
			break;
		}
	case 5: {
			switch(this->type){
			case 0: return !(*(int32_t*)data1 == *(int32_t*)data2);
			case 1: return !(*(float*)data1 == *(float*)data2);
			case 2: return !(memcmp(data1,(char*)data2 + sizeof(int32_t),length) == 0);
			}
			break;
		}
	case 6: return 1;
	}

	return 0;
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data){
	//cout << "getNextRecord Begin" << endl;
	char tempdata[PAGE_SIZE];
	RC ret = -1;
	int flag = 0;
	while (flag == 0)
	{
		ret = ph.readNextRecord(&currentRID.slotNum, tempdata);
		if (ret == 0){
			//rbfm->revertRecord(recordDescriptor,data,tempdata);
			rid.slotNum = currentRID.slotNum;
			rid.pageNum = currentRID.pageNum;
			currentRID.slotNum ++;
		}else
		if (ret == 2){
		// find real record
			rbfm->readRecord(fileHandle,recordDescriptor,currentRID,tempdata);
			//rbfm->revertRecord(recordDescriptor,data,tempdata);
			rid.slotNum = currentRID.slotNum;
			rid.pageNum = currentRID.pageNum;
			currentRID.slotNum ++;
		}else
		while(1){
			// read newpage
			currentRID.pageNum ++;
			currentRID.slotNum = 0;
			if (ph.loadPage(currentRID.pageNum, fileHandle) == -1) return RBFM_EOF;
			ret = ph.readNextRecord(&currentRID.slotNum, tempdata);
			if (ret == 0){
				//rbfm->revertRecord(recordDescriptor,data,tempdata);
				rid.slotNum = currentRID.slotNum;
				rid.pageNum = currentRID.pageNum;
				currentRID.slotNum ++;
			}else
			if (ret == 2){
				// find real record
				rbfm->readRecord(fileHandle,recordDescriptor,currentRID,tempdata);
				//rbfm->revertRecord(recordDescriptor,data,tempdata);
				rid.slotNum = currentRID.slotNum;
				rid.pageNum = currentRID.pageNum;
				currentRID.slotNum ++;
			}
		}
		int attroffset = 0;
		int attrlen = 0;
		if (this->attributenum == 0) {
			attroffset = sizeof(int16_t) * ((*(int16_t*)tempdata) + 1);
			attrlen = *((int16_t*)tempdata + 1) - attroffset;
		}
		else {
			attroffset = *((int16_t*)tempdata + attributenum);
			attrlen = *((int16_t*)tempdata + attributenum + 1) - attroffset;
		}
		void* attr = malloc(attrlen);
		memcpy(attr,(char*)tempdata + attroffset,attrlen);
		flag = this->compareRecord(attr,this->value,attrlen);
		// TODO free malloc
		free(attr);
	}
	//cout << "get One Record" << endl;
	int outputoffset = 0;
	//cout << "RID in iter: " << rid.pageNum << ", " << rid.slotNum << endl;
	//cout << "ATTRI_SIZE" << attriID.size() << endl;
	for(int i = 0; i < attriID.size(); i++){
		int32_t attrioffset = 0;
		int32_t attrilen = 0;
		if(attriID[i] == 0){
			attrioffset = sizeof(int16_t) * ((*(int16_t*)tempdata) + 1);
			attrilen = *((int16_t*)((char*)tempdata + sizeof(int16_t) )) - attrioffset;
		}else {
			attrioffset = *((int16_t*)((char*)tempdata + attriID[i]*sizeof(int16_t)));
			attrilen = *((int16_t*)((char*)tempdata + (attriID[i] + 1)*sizeof(int16_t) )) - attrioffset;
		}
		if((recordDescriptor[attriID[i]]).type != 2){
			memcpy((char*)data + outputoffset, (char*)tempdata + attrioffset, attrilen);
			outputoffset += attrilen;
		}else {
			memcpy((char*)data + outputoffset,&attrilen, sizeof(int32_t));
			outputoffset += sizeof(int32_t);
			memcpy((char*)data + outputoffset, (char*)tempdata + attrioffset, attrilen);
			outputoffset += attrilen;
		}
	}


	return ret;
}
