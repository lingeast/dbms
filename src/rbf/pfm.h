#ifndef _pfm_h_
#define _pfm_h_

#include <stdint.h>
#include <cstdio>
#include <stdexcept>
#include <map>

typedef int RC;
typedef unsigned PageNum;

#define PAGE_SIZE 4096

const int8_t INPAGE = 1;
const int8_t DELETER = -1;
const int8_t NOTINP = 0;

struct fileInfo {
	FILE* stream;
	unsigned int count;
};

// Self defined PagedFile and Page class
struct pageEntry {
	int32_t address;				// Address for the beginning for a page
	int32_t remain;					// The remaining free bytes at the end of the page
};//pageEntry for each page

const int  PAGE_DIR_SIZE = 509;

struct pageDir {					// Header page has the same page size 4096
	uint64_t pageNum;  			    // The existing pages in the directory
	int64_t next;		 			//	The address for the next directory if not exist set to -1
	int64_t dircnt;	     		    //	The directory number of this directory
	pageEntry dir[PAGE_DIR_SIZE];	// one directory contains 509 pageEntries, so the size of directory is 4096
};//page Directory containing the pageEntry for sequence pages.

const int INIT_DIR_OFFSET = 0;

class FileHandle;

class PageDirHandle {
	private:
		pageDir dirPage;
	public:
		PageDirHandle();											// empty construction
		PageDirHandle(unsigned int dirCnt);							// construction with directory number,used to construct new directory
		PageDirHandle(size_t offset ,FILE* stream);					// construction from file and directory address
		void readNewDir(size_t offset, FILE* stream);				// read and update the dirPage with directory address
		void* dataBlock() const {return (void *) &dirPage;};		// return the dirPage
		int dirCnt() const {return dirPage.dircnt;};				// return the number of directory
		int nextDir() const {return dirPage.next; };				// return the address for the next directory
		void setNextDir(int nextOff) { if(nextOff > 0) dirPage.next = nextOff;}		// set the address of the next directory
		int pageNum() const {return dirPage.pageNum; };				// return the page number in current directory
		void increPageNum(int step = 1) { dirPage.pageNum += step; };	// increase the pageNum
		pageEntry& operator[](unsigned int i)  {								// []operation overload
			if (i >= dirPage.pageNum) {
				throw std::out_of_range("PageDirHandle::operator[]");
			}
			return dirPage.dir[i];
		};
};

struct recordEntry {
	int32_t address;	// also for pagenum if migrate  		// address for record
	int16_t length;													// length for record
	int8_t occupy;													// if this Entry is occupied ,-1 for not, 1 for occupy
};

/* clase PageHandle: An abstraction of page
 * Page Format:
 * |     slot 0   |  slot 1  | slot 2 | free space --------
 * -------------------free space---------------------------
 * -------------------free space---------------------------
 * --|slotEntry2|slotEntry1|slotEntry0|free address|slotNum|
 */
class PageHandle {

	class RecordDirHandle {
	private:
		recordEntry* base;											// pointer to the recordEntry
		uint16_t* size;												// pointer to the recordNumber in the page
		uint16_t* freeAddr;											// pointer to the tail of the last reocrd in the page
	public:
		// This constructor may not be used
		RecordDirHandle():base(NULL), size(0), freeAddr(0) {;};
		// Read Dir from Page
		RecordDirHandle(PageHandle &ph);
		uint16_t* slotSize()  {return size;};
		uint16_t* free()  {return freeAddr;};
		recordEntry& operator[] (int i) const {
			if (i < 0 || i >= *size) {
				throw std::out_of_range("RecordDirHandle::operator[]");
			}
			return *(base - i);
		}

	};

private:
	int8_t data[PAGE_SIZE + sizeof(int32_t)/sizeof(int8_t)];	// to buffer page date

	/* may want to store the base address(or page ID) of this page in file
	 * Example:
	 * FileHandle.writePage(PageHandle.pageID(), PageHandle.dataBlock());
	 */
	int pageNum;
	int remaining;
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
	int getRemain() const{
		return remaining;
	}
	void * dataBlock() const { return (void *) data;};	// expose the data block, for read/write
	int readRecord(const int slotnum, void* data);	// read record with given slotnum
	unsigned insertRecord(const void* data, unsigned int length, int* newremain);	// insert record, return slot ID
	RC deleteRecord(int* slot, int* pagenum, int* newremain);
	RC updateRecord(int slot, const void* data, unsigned int length, int* newremain, int* migratePN, int* migrateSl);
	RC reorganizePage();
	RC setMigrate(int slot, int migratePN, int migrateSl, int* newremain);
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
	int getAddr(PageNum pageNum);		// Get the address of the beginnnig of the page with given page number
	int getAddrandRemain(PageNum pageNum, int* addr, int* remain);
public:
    FileHandle();                                                    // Default constructor
    ~FileHandle();                                                   // Destructor

    FILE* getFile(){return file;}									// file instance getter
    void setFile(FILE * that) {file = that;}						// file setter

    RC readPage(PageNum pageNum, void *data);                           // Get a specific page
    RC writePage(PageNum pageNum, const void *data);                    // Write a specific page
    RC appendPage(const void *data);                                    // Append a specific page
    unsigned getNumberOfPages();                                        // Get the number of pages in the file
    RC setNewremain(PageNum pageNum, int newremain);					// Update the remain attribute in the page directory
    int findfreePage(const int length, int* remain);									// find a free page for inserting Record with given length
};

 #endif
