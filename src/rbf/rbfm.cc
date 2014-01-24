
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
		rid.pageNum = fileHandle.findfreePage(length);
		if (rid.pageNum == -1) return -1;
		int newremain = 0;
		PageHandle ph(rid.pageNum, fileHandle);
		rid.slotNum = ph.insertRecord(newrecord, length, &newremain);
		free(newrecord);	// Add free() to free dynamic allocated memory, LYD JAN 24 2014
		try{
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
	length += sizeof(int32_t) * (recordDescriptor.size() + 1);
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
				cout<<"FieldContext: "<<*(int*)((char*)data + dataoffset)<<endl;
				dataoffset += sizeof(int32_t);
				break;
			case 1:
				cout<<"Recordfield: "<<i<<endl;
				cout<<"FieldType: real"<<endl;
				cout<<"FieldContext: "<<*(float*)((char*)data + dataoffset)<<endl;
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
				cout<<"FieldContext: "<<str<<endl;
				free(stringlen);
				break;
		}
		cout<<endl;
	}
	return -1;
}

void* RecordBasedFileManager::buildRecord(const vector<Attribute> &recordDescriptor,const void *data,int* recordlenth)
{
	int length = 0;
	int dataoffset = 0;
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
	void* newRecord = malloc(length + sizeof(int32_t) + sizeof(int32_t) * recordDescriptor.size());
    *((int32_t*)(newRecord)) = recordDescriptor.size();
    int32_t offset = sizeof(int32_t) + sizeof(int32_t)*recordDescriptor.size();
    dataoffset = 0;
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
		*((int32_t*)(newRecord) + i + 1) = offset;
	}
	*recordlenth = offset;
	return newRecord;
}

void RecordBasedFileManager::revertRecord(const vector<Attribute> &recordDescriptor, void *data, void* inputdata){
	int offset = 0;
	int recordoffset = sizeof(int32_t) * (*(int*)inputdata + 1);
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
				if (i == 0) strlen = *((int*)inputdata + (i+1)) - sizeof(int32_t) * (*(int*)inputdata + 1);
				else strlen = *((int*)inputdata + (i+1)) - *((int*)inputdata + i);
				memcpy((char*)data + offset, &strlen, sizeof(int32_t));
				offset += sizeof(int32_t);
				memcpy((char*)data + offset, (char*)inputdata + recordoffset, strlen);
				offset += strlen;
				recordoffset += strlen;
				break;
		}
	}
}
