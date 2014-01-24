#ifndef _pfm_h_
#define _pfm_h_

#include <stdint.h>
#include <cstdio>
#include <stdexcept>
#include <map>

typedef int RC;
typedef unsigned PageNum;

#define PAGE_SIZE 4096

struct fileInfo {
	FILE* stream;
	unsigned int count;
};

// Self defined PagedFile and Page class
struct pageEntry {
	int32_t address;
	int32_t remain;
};

const int  PAGE_DIR_SIZE = 509;

struct pageDir {	// Header page has the same page size 4096
	uint64_t pageNum;
	int64_t next;
	int64_t dircnt;
	pageEntry dir[PAGE_DIR_SIZE];
};

const int INIT_DIR_OFFSET = 0;

class FileHandle;

class PageDirHandle {
	private:
		pageDir dirPage;
	public:
		PageDirHandle();
		PageDirHandle(unsigned int dirCnt);
		PageDirHandle(size_t offset ,FILE* stream);
		void readNewDir(size_t offset, FILE* stream);
		void* dataBlock() const {return (void *) &dirPage;};
		int dirCnt() const {return dirPage.dircnt;};
		int nextDir() const {return dirPage.next; };
		void setNextDir(int nextOff) { if(nextOff > 0) dirPage.next = nextOff;}
		int pageNum() const {return dirPage.pageNum; };
		void increPageNum(int step = 1) { dirPage.pageNum += step; };
		pageEntry& operator[](int i)  {
			if (i < 0 || i >= dirPage.pageNum) {
				throw std::out_of_range("PageDirHandle::operator[]");
			}
			return dirPage.dir[i];
		};
};

struct recordEntry {
	int32_t address;
	int32_t length;
	int8_t occupy;
};

class PageHandle {

	class RecordDirHandle {
	private:
		recordEntry* base;
		uint32_t* size;
		uint32_t* freeAddr;
	public:
		// This constructor may not be used
		RecordDirHandle():base(NULL), size(0), freeAddr(0) {;};
		// Read Dir from Page
		RecordDirHandle(PageHandle &ph);
		uint32_t* slotSize()  {return size;};
		uint32_t* free()  {return freeAddr;};
		recordEntry& operator[] (int unsigned i) const {
			if (i < 0 || i >= *size) {
				throw std::out_of_range("RecordDirHandle::operator[]");
			}
			return *(base - i);
		}

	};

private:
	int8_t data[PAGE_SIZE];	// to buffer page date

	/* may want to store the base address(or page ID) of this page in file
	 * Example:
	 * FileHandle.writePage(PageHandle.pageID(), PageHandle.dataBlock());
	 */
	int pageNum;
	RecordDirHandle rdh;
public:
	/*  Init page data from file
	 *  Example:
	 *  PageHandle pg(pageNum, file);
	 *  do read or write stuff
	 *  then write back:
	 *  FileHandle.writePage(PageHandle.pageID(), PageHandle.dataBlock());
	 */
	PageHandle(int pageID, FileHandle& fh);

	/* Init empty page with a directory
	 * Example:
	 * PageHandle pg();
	 * FileHandle.appendPage(pg.dataBlock());
	 * then do some insert
	 */
	PageHandle();


	RC loadPage(int pageID, FileHandle& fh);	// load another page
	int getAddr(int slot) const { return rdh[slot].address;}	// return record address
	int pageID() const {
		if(pageNum < 0) throw new std::logic_error("Page ID unavailable for a newly-created page");
		return pageNum;
	}
	void * dataBlock() const { return (void *) data;};	// expose the data block, for read/write
	int readRecord(const int slotnum, void* data);	// read record with given slotnum
	unsigned insertRecord(const void* data, unsigned int length, int* newremain);	// insert record, return slot ID
};




class PagedFileManager
{
public:
    static PagedFileManager* instance();                     // Access to the _pf_manager instance

    RC createFile    (const char *fileName);                         // Create a new file
    RC destroyFile   (const char *fileName);                         // Destroy a file
    RC openFile      (const char *fileName, FileHandle &fileHandle); // Open a file
    RC closeFile     (FileHandle &fileHandle);                       // Close a file

protected:
    PagedFileManager();                                   // Constructor
    ~PagedFileManager();                                  // Destructor

private:
    static PagedFileManager *_pf_manager;
    std::map<int, fileInfo> fileMap;
};

class FileHandle
{

private:
	FILE* file;
	void readPageBlock(size_t offset, void* data);	// Read page from file[offset] to data
	void writePageBlock(size_t offset, const void *data);	// Write page data to file[offset]
	void writeDirBlock(size_t offset, PageDirHandle pdh); // Write Directory info pdh to file[offset]
	void moveCursor(size_t offset); 	// Call fseek(offset, file) to move cursor
	int getAddr(PageNum pageNum);
public:
    FileHandle();                                                    // Default constructor
    ~FileHandle();                                                   // Destructor

    FILE* getFile(){return file;}									// file instance getter
    void setFile(FILE * that) {file = that;}						// file setter

    RC readPage(PageNum pageNum, void *data);                           // Get a specific page
    RC writePage(PageNum pageNum, const void *data);                    // Write a specific page
    RC appendPage(const void *data);                                    // Append a specific page
    unsigned getNumberOfPages();                                        // Get the number of pages in the file
    RC setNewremain(PageNum pageNum, int newremain);
    int findfreePage(const int length);
};

 #endif
