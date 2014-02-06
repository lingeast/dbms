
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
	int pageNum = fileHandle.findfreePage(length, &newremain);
	if (pageNum == -1) {free(newrecord);return -1;};
	rid.pageNum = pageNum;
	rid.pageNum = fileHandle.findfreePage(length, &newremain);
	if (rid.pageNum == -1) {free(newrecord);return -1;};
	PageHandle ph(rid.pageNum, fileHandle);
	// insertRecord to target page
	rid.slotNum = ph.insertRecord(newrecord, length, &newremain);
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
	length = ph.readRecord(rid.slotNum, storedRecord);
	if (length == -1) return -1;
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
	return ph.deleteRecord(rid.slotNum);

}

RC RecordBasedFileManager::updateRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, const RID &rid){
	int length = 0;
	void* newrecord = buildRecord(recordDescriptor, data, &length);
	PageHandle ph(rid.pageNum,fileHandle);


}

RC RecordBasedFileManager::reorganizePage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const unsigned pageNumber){
	PageHandle ph(pageNumber,fileHandle);
	int32_t newremain = 0;
	newremain = ph.reorganizePage();
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
