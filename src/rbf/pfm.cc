#include "pfm.h"
#include <iostream>
#include <cassert>
PageDirHandle::PageDirHandle() {
	PageDirHandle(0);
}
PageDirHandle::PageDirHandle(unsigned int dirCnt) {
	// Init new dirPage
	dirPage.dircnt = dirCnt;
	dirPage.next = -1;
	dirPage.pageNum = 0;
	for (int i = 0; i < PAGE_DIR_SIZE; i++) {
		dirPage.dir[i].address = -1;
	}
}

PageDirHandle::PageDirHandle(size_t offset, FILE* stream) {
	readNewDir(offset, stream);
}

void PageDirHandle::readNewDir(size_t offset, FILE* stream) {
	int ret = fseek(stream, offset, SEEK_SET);
	assert(ret == 0);

	ret = fread(&dirPage, sizeof(dirPage), 1, stream);
	assert(ret == 1);
}
PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const char *fileName)
{
	FILE* file;
	file = fopen(fileName, "r");
	if (file != NULL) {
		fclose(file);
		return -1;
	} else {
		file = fopen(fileName, "w");
		if (file == NULL)
			return -1;
		else {
			//TODO: do some init work to the new file, (ex. set up directory page)
			PageDirHandle pd(0);
			int ret = fwrite(pd.dataBlock(), sizeof(pageDir), 1, file);
			if (ret != 1) return -1;
			fclose(file);
			return 0;
		}
	}
}


RC PagedFileManager::destroyFile(const char *fileName)
{
	return remove(fileName);
}


RC PagedFileManager::openFile(const char *fileName, FileHandle &fileHandle)
{
	FILE* file;
	file = fopen(fileName, "r+");

	if (file == NULL)
		return -1;

	fileHandle.setFile(file);
	return 0;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
	int ret =  fclose(fileHandle.getFile());
	fileHandle.setFile(NULL);
	return ret;
}


FileHandle::FileHandle() : file(NULL)
{
}


FileHandle::~FileHandle()
{
}

void FileHandle::moveCursor(size_t offset) {	// Calling fseek to move file pointer
	assert (file != NULL);
	if (fseek(file, offset, SEEK_SET) != 0)
		throw std::runtime_error("FileHandle: Move cursor failed.");
}

void FileHandle::readPageBlock(size_t offset, void* data) {
	assert(file != NULL);
	moveCursor(offset);
	if (1 != fread(data, PAGE_SIZE, 1, file))
		throw std::runtime_error("FileHandle: Read page block failed.");
}

void FileHandle::writePageBlock(size_t offset, const void *data) {
	assert(file != NULL);
	moveCursor(offset);
	if(1 != fwrite(data, PAGE_SIZE, 1, file))
		throw std::runtime_error("FileHandle: Write page block failed.");
}

void FileHandle::writeDirBlock(size_t offset, PageDirHandle pdh) {
	assert(file != NULL);
	moveCursor(offset);
	if( 1 != fwrite(pdh.dataBlock(), sizeof(pageDir), 1, file))
		throw std::runtime_error("FileHandle: Write dirblock failed.");

}


int FileHandle::getAddr(PageNum pageNum) {
	PageDirHandle pdh(INIT_DIR_OFFSET, file);
	int pageIndex = pageNum;
	while(pageIndex >= PAGE_DIR_SIZE) {
		pdh.readNewDir(pdh.nextDir(), file);
		pageIndex -= PAGE_DIR_SIZE;
	}
	return pdh[pageIndex].address;
}

RC FileHandle::readPage(PageNum pageNum, void *data)
{
	int offset = getAddr(pageNum);
	if (offset < 0) return -1;
	try {
		readPageBlock(offset, data);
	} catch (const std::runtime_error& error) {
		return -1;
	}
	// Read Succeed
	return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
	int offset = getAddr(pageNum);
	if (offset < 0 ) return -1;
	try {
		writePageBlock(offset, data);
	} catch (const std::runtime_error& error) {
		return -1;
	}
	// Write Succeed
	return 0;
}


RC FileHandle::appendPage(const void *data)
{
	PageDirHandle pdh(INIT_DIR_OFFSET, file);
	while(pdh.pageNum() == PAGE_DIR_SIZE) {// This directory is full
		if (pdh.nextDir() < 0) {
			// next dir doesn't exists, create a new one
			PageDirHandle newPdh(pdh.dirCnt() + 1);
			// Write newly-created dir
			int offset = newPdh.dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE);
			writeDirBlock(offset, newPdh);
			pdh.setNextDir(offset);

			// Write back old one
			writeDirBlock(pdh.dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE), pdh);
		}
		pdh.readNewDir(pdh.nextDir(), file);
	}

	int dirOffset = ftell(file) - sizeof(pageDir);	// Store directory for future writeback
	int i = 0;
	while(i < pdh.pageNum() && pdh[i].address != -1)
		++i;

	// pdh[i] is empty
	pdh.increPageNum();	// update PageNum
	pdh[i].remain = PAGE_SIZE; //TODO: questionable behavior
	pdh[i].address = pdh.dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE)	//one dir + PAGE_DIR_SIZE pages
			+ sizeof(pageDir) + PAGE_SIZE * i;


	try {
		writePageBlock(pdh[i].address, data);
	} catch (const std::runtime_error& error) {
		//std::cerr << error << std::endl;
		return -1;
	}

	// If write page successfully, writeback directory
	writeDirBlock(dirOffset, pdh);
	return 0;
}


unsigned FileHandle::getNumberOfPages()
{
	PageDirHandle pdh(INIT_DIR_OFFSET, file);
	int num = 0;
	while(true) {
		num += pdh.pageNum();
		int nextDir = pdh.nextDir();
		if (nextDir < 0) break;
		else pdh.readNewDir(nextDir, file);
	}
    return num;
}


