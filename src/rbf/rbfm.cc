
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
		rid.slotNum = ph.insertRecord(newrecord,length,&newremain);
		try{
		fileHandle.writePage(rid.pageNum, ph.dataBlock());
		}catch(const std::exception &e){
			std::cout<< e.what() << std::endl;
			return -1;
		}
		fileHandle.setNewremain(rid.pageNum,newremain);
		return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	PageHandle ph(rid.pageNum,fileHandle);
	int length = 0;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				length += sizeof(int);
				break;
			case 1:
				length += sizeof(float);
				break;
			case 2:
				length += recordDescriptor[i].length;
				break;
		}
	}
	length += sizeof(int) * (recordDescriptor.size()+1);
	void* storedRecord = malloc(length);
	length = ph.readRecord(rid.slotNum, storedRecord);
	if (length == -1) return -1;
	revertRecord(recordDescriptor,data,storedRecord);
	free(storedRecord);

    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
	int dataoffset = 0;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				cout<<*(int*)((char*)data+dataoffset)<<endl;
				dataoffset += sizeof(int);
				break;
			case 1:
				cout<<*(float*)((char*)data+dataoffset)<<endl;
				dataoffset += sizeof(float);
				break;
			case 2:
				int* stringlen = new int;
				memcpy(stringlen,(char*)data + dataoffset, sizeof(int));
				char* str = (char*)(malloc(sizeof(char)*(*stringlen + 1)));
				memcpy(str,(char*)data + dataoffset + sizeof(int),*stringlen);
				dataoffset += *stringlen + sizeof(int);
				cout<<str<<endl;
				free(stringlen);
				break;
		}
	}
	return -1;
}

void* RecordBasedFileManager::buildRecord(const vector<Attribute> &recordDescriptor,const void *data,int* recordlenth)
{
	int length = 0;
	int dataoffset = 0;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				length += sizeof(int);
				dataoffset += sizeof(int);
				break;
			case 1:
				length += sizeof(float);
				dataoffset += sizeof(float);
				break;
			case 2:
				int* stringlen = new int;
				memcpy(stringlen,(char*)data + dataoffset, sizeof(int));
				length += *stringlen;
				dataoffset += *stringlen + sizeof(int);
				free(stringlen);
				break;
		}
	}
	void* newRecord = malloc(length + sizeof(int) + sizeof(int)*recordDescriptor.size());
    *((int32_t*)(newRecord)) = recordDescriptor.size();
    int32_t offset = sizeof(int) + sizeof(int)*recordDescriptor.size();
    dataoffset = 0;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ )
	{
		switch((recordDescriptor[i]).type)
		{
			case 0:
				memcpy((char*)(newRecord)+offset, (char*)(data) + dataoffset, sizeof(int));
				offset += sizeof(int);
				dataoffset += sizeof(int);
				break;
			case 1:
				memcpy((char*)(newRecord)+offset, (char*)(data) + dataoffset, sizeof(float));
				offset += sizeof(float);
				dataoffset += sizeof(float);
				break;
			case 2:
				int *stringlen = new int;
				memcpy(stringlen, (char*)(data) + dataoffset, sizeof(int));
				memcpy((char*)(newRecord)+offset, (char*)(data) + dataoffset + sizeof(int), *stringlen);
				offset += *stringlen;
				dataoffset += sizeof(int) + *stringlen;
				free(stringlen);
				break;
		}
		*((int32_t*)(newRecord) + i + 1) = offset;
	}
	//cout<<"testlen"<<*(int*)(((char*)newRecord + 30*4))<<endl;
	*recordlenth = offset;
	return newRecord;
}

void RecordBasedFileManager::revertRecord(const vector<Attribute> &recordDescriptor, void *data, void* inputdata){
	int offset = 0;
	int recordoffset = sizeof(int) * (*(int*)inputdata + 1);
	//cout<<"offset:"<<recordoffset<<endl;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ ){
		switch((recordDescriptor[i]).type)
		{
			case 0:
				memcpy((char*)data + offset,(char*)inputdata + recordoffset,sizeof(int));
				offset += sizeof(int);
				recordoffset += sizeof(int);
				break;
			case 1:
				memcpy((char*)data + offset,(char*)inputdata + recordoffset,sizeof(float));
				offset += sizeof(float);
				recordoffset += sizeof(float);
				break;
			case 2:
				int strlen = 0;
				if (i == 0) strlen = *((int*)inputdata + (i+1)) - sizeof(int) * (*(int*)inputdata + 1);
				else strlen = *((int*)inputdata + (i+1)) - *((int*)inputdata + i);
				memcpy((char*)data + offset, &strlen, sizeof(int));
				offset += sizeof(int);
				memcpy((char*)data + offset, (char*)inputdata + recordoffset, strlen);
				offset += strlen;
				recordoffset += strlen;
				break;
		}
	}
}
