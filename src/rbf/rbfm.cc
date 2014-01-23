
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

RC RecordBasedFileManager::insertRecord(const FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
		void* newrecord = buildRecord(recordDescriptor,data);
		rid = fileHandle.findfreePage(newrecord,sizeof(newrecord));
		return 0;
	/*
	 * //Transform raw record into variable length record format
	 * DataBuffer buffer(recordDescriptor, data);
	 * // Databuffer is a class that create & store record format data
	 *
	 *
	 *	// Find a Page for insertion
	 * rid.pageNum = fileHandle.findWritablePage();
	 *
	 *	// Read Page
	 * PageHandle pg(pageNum, fileHandle);
	 *	// Read Dir
	 * PageDirHandle pdh(pageNum, fileHandle);
	 *
	 *	// Insert into page
	 * rid.slotNum = pg.insertRecord(buffer.data(), buffer.length());
	 *
	 *	// Write back page
	 * fileHandle.writePage(pg.pageID, pg.dataBlock());
	 *	// Write back dir
	 * fileHandle.writeDirBlock(dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE, pdh);
	 *
	 */
    return -1;
}

RC RecordBasedFileManager::readRecord(const FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
	/*
	 *	PageHandle pg(rid.pageNum, fileHandle);
	 *
	 *	char * buffer;
	 *
	 *	pg.readRecord(rid.slotNum, buffer);
	 *
	 *	TransformBack(buffer, data);
	 *
	 *	delete buffer;
	 */
    return -1;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
    return -1;
}

void* buildRecord(const vector<Attribute> &recordDescriptor,const void *data)
{
	int length = 0;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ )
		length+=(recordDescriptor[i]).length;
	void* newRecord=malloc(length+4*recordDescriptor.size()+4);
    *((int32_t*)(newRecord)) = recordDescriptor.size();
    int32_t offset = 4+4*recordDescriptor.size();
	int32_t dataoffset = 0;
	for (int i = 0 ; i < recordDescriptor.size() ; i++ )
	{
		*((int32_t*)(newRecord)+i+1) = offset;
		void* newfield = ((char*)(newRecord)+offset);
		void* datafield = ((char*)(data)+dataoffset);
		switch((recordDescriptor[i]).type)
		{
			case 0:
				*((int32_t*)(newfield)) = *((int32_t*)(datafield));
				break;
			case 1:
				*((float*)(newfield)) = *((float*)(datafield));
				break;
			case 2:
				*((string*)(newfield)) = *((string*)(datafield));
				break;
		}
		offset += (recordDescriptor[i]).length;
		dataoffset += (recordDescriptor[i]).length;
	}
	return newRecord;
}
