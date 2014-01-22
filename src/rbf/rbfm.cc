
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
    return -1;
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return -1;
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return -1;
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return -1;
}

RC RecordBasedFileManager::insertRecord(const FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
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
