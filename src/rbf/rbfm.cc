
#include "rbfm.h"


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
	return PagedFileManager::instance()->openFile(fileName.c_str(),fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return PagedFileManager::instance()->closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {

	int length = 0;
	void* newrecord = buildRecord(recordDescriptor, data, &length);
	// find page to insert the record
	int newremain = 0;
	rid.pageNum = fileHandle.findfreePage(length, &newremain);
	if (rid.pageNum == -1) {free(newrecord);return -1;};
	PageHandle ph(rid.pageNum, fileHandle);
	// insertRecord to target page
	rid.slotNum = ph.insertRecord(newrecord, length, &newremain,0);
	// if need reorganize
	if (rid.slotNum == -1) {
		this->reorganizePage(fileHandle,recordDescriptor,rid.pageNum);
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
			exactrid.pageNum = *((int32_t*) data);
			exactrid.slotNum = *((int16_t*)data + sizeof(int32_t)/sizeof(int16_t));
			ph.loadPage(exactrid.pageNum,fileHandle);
		}
		length = ph.readRecord(exactrid.slotNum, storedRecord);
	}while(length == 0);
	if (length < 0) return -1;
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
				break;
		}
		cout<<endl;
	}
	return -1;
}

RC RecordBasedFileManager::deleteRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid){
	PageHandle ph(rid.pageNum,fileHandle);
	RID exactrid = rid;
	RC result;
	int newremain = 0;
	result = ph.deleteRecord(&exactrid.slotNum, &exactrid.pageNum, &newremain);
	// if migrate to other page
	while(result == 1){
		try{
				// write back page and page remaining info
				fileHandle.writePage(exactrid.pageNum, ph.dataBlock());
				//fileHandle.setNewremain(rid.pageNum,newremain);
			}catch(const std::exception &e){
				std::cout<< e.what() << std::endl;
				return -1;
		}

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
			this->reorganizePage(fileHandle,recordDescriptor,newrid.pageNum);
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
	void * record = malloc(PAGE_SIZE);
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
	cout<<"test1"<<endl;
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

}

RBFM_ScanIterator::~RBFM_ScanIterator(){

}

RC RBFM_ScanIterator::close() {
	return rbfm->closeFile(fileHandle);
}

void RBFM_ScanIterator::setIterator(FileHandle &fileHandle,
	      const vector<Attribute> &recordDescriptor,
	      const string &conditionAttribute,
	      const CompOp compOp,                  // comparision type such as "<" and "="
	      const void *value,                    // used in the comparison
	      const vector<string> &attributeNames, // a list of projected attributes
	      RecordBasedFileManager *rbfm
	      ){
	this->fileHandle = fileHandle;
	this->recordDescriptor = recordDescriptor;
	this->conditionAttribute = conditionAttribute;
	this->compOp = compOp;
	this->value = value;
	this->attributeNames = attributeNames;
	this->ph.loadPage(0,fileHandle);
	this->currentRID.pageNum = 0;
	this->currentRID.slotNum = 0;
	this->rbfm = rbfm;
}

RC RBFM_ScanIterator::getNextRecord(RID &rid, void *data){
	void* tempdata = malloc(PAGE_SIZE);
	RC ret = ph.readNextRecord(&currentRID.slotNum, tempdata);
	if (ret == 0){
		rbfm->revertRecord(recordDescriptor,data,tempdata);
		rid.slotNum = currentRID.slotNum;
		rid.pageNum = currentRID.pageNum;
		currentRID.slotNum ++;
		return 0;
	}else
	if (ret == 2){
		// find real record
		rbfm->readRecord(fileHandle,recordDescriptor,currentRID,tempdata);
		rbfm->revertRecord(recordDescriptor,data,tempdata);
		rid.slotNum = currentRID.slotNum;
		rid.pageNum = currentRID.pageNum;
		currentRID.slotNum ++;
		return 0;
	}else
	while(1){
		// read newpage
		currentRID.pageNum ++;
		currentRID.slotNum = 0;
		if (ph.loadPage(currentRID.pageNum, fileHandle) == -1) return RBFM_EOF;
		ret = ph.readNextRecord(&currentRID.slotNum, tempdata);
		if (ret == 0){
			rbfm->revertRecord(recordDescriptor,data,tempdata);
			rid.slotNum = currentRID.slotNum;
			rid.pageNum = currentRID.pageNum;
			currentRID.slotNum ++;
			return 0;
		}else
		if (ret == 2){
			// find real record
			rbfm->readRecord(fileHandle,recordDescriptor,currentRID,tempdata);
			rbfm->revertRecord(recordDescriptor,data,tempdata);
			rid.slotNum = currentRID.slotNum;
			rid.pageNum = currentRID.pageNum;
			currentRID.slotNum ++;
			return 0;
		}
	}
	return RBFM_EOF;
}
