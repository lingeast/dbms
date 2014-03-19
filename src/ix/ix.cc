
#include "ix.h"
#include <cstdio>

#include "../bpt/key/intkey.h"
#include "../bpt/key/floatkey.h"
#include "../bpt/key/varcharkey.h"

using BPlusTree::bt_key;

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
		return 0;
	}
}

RC IndexManager::destroyFile(const string &fileName)
{
	if (bTreeMap.find(fileName) != bTreeMap.end()) {
		std::cout << "The fileHandle has not been closed yet" << std::endl;
		return -1;
	} else {
		cout << "Try Destroy file " << fileName << endl;
		return PagedFileManager::instance()->destroyFile(fileName.c_str());
	}
}

RC IndexManager::openFile(const string &fileName, FileHandle &fileHandle)
{
	if(PagedFileManager::instance()->openFile(fileName.c_str(), fileHandle) != 0)
		return -1;
	// insert  bp_tree obj
	std::pair<std::map<string,bp_tree>::iterator,bool> ret;
	ret = bTreeMap.insert(std::pair<string,bp_tree>(fileName, fileName.c_str()));


	// Index file does not need file handle!
	fileHandle.setFile(NULL);

	//cout << "Map Size = " << bTreeMap.size() << endl;
	if (ret.second){
		return 0;
	} else {
		//std::cout << "Why want to open same btree file more than once?" << std::endl;
		return 0;
	}
	std::cout << "Never reach here" << std::endl;
	return 0;

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
	std::map<string,bp_tree>::iterator bpt = bTreeMap.find(fileHandle.getFileName());
	if (bpt == bTreeMap.end()) {
		cout << "fileHandle not opened by IndexManager" << endl;
	}

	bt_key* insert_key = NULL;

	switch(attribute.type) {
	case TypeInt:
		insert_key = new int_key();
		break;
	case TypeReal:
		insert_key = new float_key();
		break;
	case TypeVarChar:
		insert_key = new varchar_key();
		break;
	}
	insert_key->load(key);

	cout << "Insert " << insert_key->to_string() << "(" << insert_key->length() << ")"<< " to " << bpt->first << endl;
	RC ret = bpt->second.insert_entry(insert_key, rid);

	if (insert_key != NULL) delete insert_key;
	cout << "Ret value = " << ret << endl;
	return ret;
}

RC IndexManager::deleteEntry(FileHandle &fileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
	std::map<string,bp_tree>::iterator bpt = bTreeMap.find(fileHandle.getFileName());
	if (bpt == bTreeMap.end()) {
		cout << "fileHandle not opened by IndexManager" << endl;
	}

	bt_key* delete_key = NULL;

	switch(attribute.type) {
	case TypeInt:
		delete_key = new int_key();
		break;
	case TypeReal:
		delete_key = new float_key();
		break;
	case TypeVarChar:
		delete_key = new varchar_key();
		break;
	}
	delete_key->load(key);
	return bpt->second.delete_entry(delete_key, rid);

}

RC IndexManager::scan(FileHandle &fileHandle,
    const Attribute &attribute,
    const void      *lowKey,
    const void      *highKey,
    bool			lowKeyInclusive,
    bool        	highKeyInclusive,
    IX_ScanIterator &ix_ScanIterator)
{
	//cout << "IM::IndexScan on " << fileHandle.getFileName() << " ," << attribute.name << endl;
	FILE* tmp = fopen(fileHandle.getFileName().c_str(), "r");

	if (tmp == NULL){ // check if file exists
		cout << "IM::scan, file missing" << endl;
		return -1;
	} else {
		fclose(tmp);
		tmp = NULL;
	}

	bt_key* lowk = NULL;
	bt_key* hik = NULL;

	switch(attribute.type) {
	case TypeInt:
		lowk = new int_key();
		hik = new int_key();
		break;
	case TypeReal:
		lowk = new float_key();
		hik = new float_key();
		break;
	case TypeVarChar:
		lowk = new varchar_key();
		hik = new varchar_key();
		break;
	}

	if (lowKey != NULL) {
		lowk->load(lowKey);
	} else {
		lowk->set_inf(-1);
	}

	if (highKey != NULL) {
		hik->load(highKey);
	} else {
		hik->set_inf(1);
	}

	// A newly created B+ tree object to be passed into ix_ScanIterator
	// its life cycle controlled by ix_ScanIterator object

	bpt_scan_itr* bsi = new bpt_scan_itr(fileHandle.getFileName(), lowk, hik,
			lowKeyInclusive, highKeyInclusive);

	try {
		ix_ScanIterator.set_itr(bsi);
	} catch (const exception& e) {
		return -1;
	}
	return 0;
}

IX_ScanIterator::IX_ScanIterator() : scan_itr(NULL)
{
}

IX_ScanIterator::~IX_ScanIterator()
{
	if (scan_itr != NULL) delete scan_itr;
	scan_itr = NULL;
}
void IX_ScanIterator::set_itr(bpt_scan_itr* that) {
	  if (that == NULL)
		  throw new std::logic_error("IX_ScanIterator need non-null ptr");
	  this->scan_itr = that;
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
	return this->scan_itr->get_next(rid, key);
}

RC IX_ScanIterator::close()
{
	if (scan_itr != NULL) {
		delete scan_itr;
		scan_itr = NULL;
		return 0;
	} else {
		// has closed before or has not inited yet
		return -1;
	}

}

void IX_PrintError (RC rc)
{
}
