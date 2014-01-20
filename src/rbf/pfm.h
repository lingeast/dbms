#ifndef _pfm_h_
#define _pfm_h_

#include <stdint.h>
#include <cstdio>
#include <stdexcept>

typedef int RC;
typedef unsigned PageNum;

#define PAGE_SIZE 4096


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
	int pageNum() const {return dirPage.pageNum; };
	void increPageNum(int step = 1) { dirPage.pageNum += step; };
	pageEntry& operator[](int i)  {
		if (i < 0 || i >= dirPage.pageNum) {
			throw std::out_of_range("PageDirHandle::operator[]");
		}
		return dirPage.dir[i];
	};
	//TODO: add more function so that multiple dir page can be treated as a integrated one
};

class FileHandle;


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
 };

 #endif
