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

PageHandle::RecordDirHandle::RecordDirHandle(PageHandle ph) {
	uint32_t* dirReader = (uint32_t*) ph.dataBlock();
	size =  dirReader + (PAGE_SIZE / sizeof(int32_t)) - 1;	// get the address of size
	freeAddr = dirReader + (PAGE_SIZE / sizeof(int32_t)) - 2;	// get the address of freeAddr
	int8_t const* baseReader = (int8_t *) ph.dataBlock();
	base = ((recordEntry*) freeAddr) - 1;

	/*
	baseEnd = (recordEntry*) (baseReader + PAGE_SIZE
			- (1 + 1) * sizeof(int32_t)	// memory to store size and free offset
			- (*size) * sizeof(recordEntry));	// memory to store directory entry
			*/
}

PageHandle::PageHandle(int pageID, FileHandle& fh) {
	pageNum = pageID;
	fh.readPage(pageNum, this->data);
	rdh = RecordDirHandle(*this);
}

PageHandle::PageHandle():pageNum(-1) {
	uint32_t* intReader = (uint32_t*) data;
	intReader[(PAGE_SIZE / sizeof(int32_t)) - 1] = 0; // set size = 0
	intReader[(PAGE_SIZE / sizeof(int32_t)) - 2] = 0; // set freeAddr = 0
}

RC PageHandle::loadPage(int pageID, FileHandle& fh) {
	pageNum = pageID;
	try {
		fh.readPage(pageNum, this->data);
		rdh = RecordDirHandle(*this);
	} catch (const std::exception& e) {
		std::cout<< e.what() << std::endl;
		return -1;
	}
	return 0;
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
	if (_pf_manager)
		delete _pf_manager;
}


RC PagedFileManager::createFile(const char *fileName)
{
	FILE* file;
	file = fopen(fileName, "rb");
	if (file != NULL) {
		fclose(file);
		return -1;
	} else {
		file = fopen(fileName, "wb");
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
	file = fopen(fileName, "r+b");

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
	// Check if PagedFileManager forget to close the file
	if (file != NULL)
		fclose(file);
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
	} catch (const std::exception& e) {
		std::cout<< e.what() << std::endl;
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
	} catch (const std::exception& e) {
		std::cout<< e.what() << std::endl;
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
			try {
				writeDirBlock(offset, newPdh);
			} catch (const std::exception& e) {
						std::cout<< e.what() << std::endl;
						return -1;
			}

			// Update nextDir field in old one
			pdh.setNextDir(offset);

			// Write back old one
			try {
				writeDirBlock(pdh.dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE), pdh);
			} catch (const std::exception& e) {
				std::cout<< e.what() << std::endl;
				return -1;
			}
		}
		pdh.readNewDir(pdh.nextDir(), file);
	}

	int dirOffset = pdh.dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE); // Store directory for future writeback
			//ftell(file) - sizeof(pageDir); Stop using ftell beacuse FILE* file may be moved by other FileHandle
	int i = 0;
	while(i < pdh.pageNum() && pdh[i].address != -1)
		++i;

	// pdh[i] is empty
	pdh.increPageNum();	// update PageNum
	pdh[i].remain = PAGE_SIZE; //TODO: questionable behavior
	pdh[i].address = pdh.dirCnt() * (sizeof(pageDir) + PAGE_DIR_SIZE * PAGE_SIZE)	//one dir + PAGE_DIR_SIZE pages
			+ sizeof(pageDir) + PAGE_SIZE * i;

	// Write page
	try {
		writePageBlock(pdh[i].address, data);
	} catch (const std::exception& error) {
		std::cerr << error.what() << std::endl;
		return -1;
	}

	// If write page successfully, writeback directory
	try {
		writeDirBlock(dirOffset, pdh);
	} catch (const std::exception& e) {
		std::cout<< e.what() << std::endl;
		return -1;
	}
	// Succeed, return 0
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


