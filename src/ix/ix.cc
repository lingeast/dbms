
#include "ix.h"
#include <cstdio>
IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
}

IndexManager::~IndexManager()
{
}

RC IndexManager::createFile(const string &fileName)
{
	FILE* tmp = fopen(fileName.c_str(), "r");
	if (tmp != NULL) {
		fclose(tmp);
		return -1;
	} else {
		// bp_tree constructor will auto create a feil
		BPlusTree::bp_tree(fileName.c_str());
	}
}

RC IndexManager::destroyFile(const string &fileName)
{
	return -1;
}

RC IndexManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	if(PagedFileManager::instance()->openFile(fileName.c_str(), fileHandle) != 0)
		return -1;
	// insert  bp_tree obj
	std::pair<std::map<string,bp_tree>::iterator,bool> ret;
	ret = bTreeMap.insert(std::pair<string,bp_tree>(fileName, fileName.c_str()));
	if (ret.second) return 0;
	else {
		std::cout << "Why want to open same btree file more than once?" << std::endl;
	}
}

RC IndexManager::closeFile(FileHandle &fileHandle)
{
	int ret = bTreeMap.erase(fileHandle.getFileName());
	if (ret != 1) {
		throw new std::logic_error("BTreeFile Map does not have this file!");
	}
	return PagedFileManager::instance()->closeFile(fileHandle);
}

RC IndexManager::insertEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	return -1;
}

RC IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	return -1;
}

RC IndexManager::scan(FileHandle &fileHandle,
    const Attribute &attribute,
    const void      *lowKey,
    const void      *highKey,
    bool			lowKeyInclusive,
    bool        	highKeyInclusive,
    IX_ScanIterator &ix_ScanIterator)
{
	return -1;
}

IX_ScanIterator::IX_ScanIterator()
{
}

IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	return -1;
}

RC IX_ScanIterator::close()
{
	return -1;
}

void IX_PrintError (RC rc)
{
}
