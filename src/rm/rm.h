
#ifndef _rm_h_
#define _rm_h_

#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>

#include "../ix/ix.h"
#include "../rbf/rbfm.h"

using namespace std;

unsigned int const RAW_LEN_SIZE = 4;
unsigned int const RAW_INT_SIZE = 4;

# define RM_EOF (-1)  // end of a scan operator

// RM_IndexScanIterator scan the index of a given table
class RM_IndexScanIterator {
private:
	IX_ScanIterator* ix_scan;
 public:
  RM_IndexScanIterator();  	// Constructor
  ~RM_IndexScanIterator(); 	// Destructor

  // "key" follows the same format as in IndexManager::insertEntry()
  RC getNextEntry(RID &rid, void *key); 	// Get next matching entry
  RC close();            			// Terminate index scan

  void set_itr(IX_ScanIterator* that_scan);
};

// RM_ScanIterator is an iteratr to go through tuples
// The way to use it is like the following:
//  RM_ScanIterator rmScanIterator;
//  rm.open(..., rmScanIterator);
//  while (rmScanIterator(rid, data) != RM_EOF) {
//    process the data;
//  }
//  rmScanIterator.close();

class RM_ScanIterator {
private:
	RBFM_ScanIterator RBFM_it;
public:
  RM_ScanIterator() {};
  ~RM_ScanIterator() {};
  void setIterator(FileHandle &fileHandle,
  	      const vector<Attribute> &recordDescriptor,
  	      const string &conditionAttribute,
  	      const CompOp compOp,                  // comparision type such as "<" and "="
  	      const void *value,                    // used in the comparison
  	      const vector<string> &attributeNames, // a list of projected attributes
  	      RecordBasedFileManager *rbfm) {
	  RBFM_it.setIterator(fileHandle, recordDescriptor, conditionAttribute,
			  compOp, value, attributeNames, rbfm);

  };
  /*
  RC loadConfig(RecordBasedFileManager* sing_rbfm,
  			const vector <Attribute>& recordDescriptor,
  			const FileHandle& fileH);
  			*/
  // "data" follows the same format as RelationManager::insertTuple()
  RC getNextTuple(RID &rid, void *data) { return RBFM_it.getNextRecord(rid, data); };
  RC close() { return RBFM_it.close(); };
};


// Relation Manager
class RelationManager
{
	class columnRecord {
	private:
		char* data;

	public:
		static const int COLTYPE_STR_LEN = 6;
		void* recordData() const {return data;};
		columnRecord(const string& tableName, const Attribute &attr, unsigned int pos)
		: data(NULL) {
			uint32_t name_len = tableName.size();
			uint32_t col_name_len = attr.name.size();

			const char TYPE_STRING[] = "string";
			const char TYPE_INT[] = "int   ";
			const char TYPE_REAL[] = "real  ";
			string colType;
			switch(attr.type) {
			case TypeVarChar:
				colType = TYPE_STRING;
				break;
			case TypeInt:
				colType = TYPE_INT;
				break;
			case TypeReal:
				colType = TYPE_REAL;
				break;
			}

			uint32_t type_len = colType.size();
			uint32_t colPosition = pos;
			uint32_t maxSize = attr.length;

			data =(char*) malloc(RAW_LEN_SIZE + name_len // tableName
					+ RAW_LEN_SIZE + col_name_len	// colName
					+ RAW_LEN_SIZE + type_len		//colType
					+ RAW_INT_SIZE 					//colPosition
					+ RAW_INT_SIZE);				//maxSize

			assert(colType.size() == COLTYPE_STR_LEN);
			assert(RAW_INT_SIZE == sizeof(colPosition));
			assert(RAW_LEN_SIZE == sizeof(type_len));
			char* dst = data;
			// tableName
			memcpy(dst, &name_len, RAW_LEN_SIZE);
			dst += RAW_LEN_SIZE;
			memcpy(dst, tableName.c_str(), name_len);
			dst += name_len;

			// colName
			memcpy(dst, &col_name_len, RAW_LEN_SIZE);
			dst += RAW_LEN_SIZE;
			memcpy(dst, attr.name.c_str(), col_name_len);
			dst += col_name_len;

			//colType
			memcpy(dst, &type_len, RAW_LEN_SIZE);
			dst += RAW_LEN_SIZE;
			memcpy(dst, colType.c_str(), type_len);
			dst += type_len;

			//colPosition
			memcpy(dst, &colPosition, RAW_INT_SIZE);
			dst += RAW_INT_SIZE;

			// maxSize
			memcpy(dst, &maxSize, RAW_INT_SIZE);
			dst += RAW_INT_SIZE;



		};
		~columnRecord() {
			if (data)
				free(data);
		};
	};
	class tableRecord {
	private:
		char* data;
	public:
		void* recordData() const {return data;};
		tableRecord(const string& tableName, const string& tableType, const vector<Attribute> &attrs):data(NULL) {
			uint32_t name_len = tableName.size();
			uint32_t type_len = tableType.size();
			assert(RAW_LEN_SIZE == sizeof(uint32_t));
			data =(char*) malloc(RAW_LEN_SIZE + name_len	// tableName
					+ RAW_LEN_SIZE + type_len				//tableType
					+ RAW_INT_SIZE);						//numCol
			char* dst = data;
			memcpy(dst, &name_len, RAW_LEN_SIZE);
			dst += RAW_LEN_SIZE;
			memcpy(dst, tableName.c_str(), name_len);
			dst += name_len;

			memcpy(dst, &type_len, RAW_LEN_SIZE);
			dst += RAW_LEN_SIZE;
			memcpy(dst, tableType.c_str(), type_len);
			dst += type_len;

			uint32_t colNum = attrs.size();
			assert(sizeof(colNum) == RAW_INT_SIZE);
			memcpy(dst, &colNum, RAW_INT_SIZE);

		};
		~tableRecord() {
			if (data)
				free(data);
		};
	};
private:
	vector<Attribute> tblRecord;
	vector<Attribute> colRecord;
	string TBL_T_SYSTEM; // table type system

	int getTableColNum(const string &tableName);
	/*
	 * return value: the column position of this attribute
	 */
	int fillAttr(void* record, Attribute& attr);

	string indexName(const string& tableName, const string& attrName);

	Attribute getAttr(const string &tableName, const string &attributeName);
public:
RecordBasedFileManager* rbfm;
  static RelationManager* instance();

  RC createTable(const string &tableName, const vector<Attribute> &attrs);

  RC deleteTable(const string &tableName);

  RC getAttributes(const string &tableName, vector<Attribute> &attrs);

  RC insertTuple(const string &tableName, const void *data, RID &rid);

  RC deleteTuples(const string &tableName);

  RC deleteTuple(const string &tableName, const RID &rid);

  // Assume the rid does not change after update
  RC updateTuple(const string &tableName, const void *data, const RID &rid);

  RC readTuple(const string &tableName, const RID &rid, void *data);

  RC readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data);

  RC reorganizePage(const string &tableName, const unsigned pageNumber);

  // scan returns an iterator to allow the caller to go through the results one by one. 
  RC scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  // comparision type such as "<" and "="
      const void *value,                    // used in the comparison
      const vector<string> &attributeNames, // a list of projected attributes
      RM_ScanIterator &rm_ScanIterator);


// Extra credit
public:
  RC dropAttribute(const string &tableName, const string &attributeName);

  RC addAttribute(const string &tableName, const Attribute &attr);

  RC reorganizeTable(const string &tableName);

// Project 4
  RC createIndex(const string &tableName, const string &attributeName);

  RC destroyIndex(const string &tableName, const string &attributeName);

  // indexScan returns an iterator to allow the caller to go through qualified entries in index
  RC indexScan(const string &tableName,
                        const string &attributeName,
                        const void *lowKey,
                        const void *highKey,
                        bool lowKeyInclusive,
                        bool highKeyInclusive,
                        RM_IndexScanIterator &rm_IndexScanIterator
       );


protected:
  RelationManager();
  ~RelationManager();

private:
  static RelationManager *_rm;
};

#endif
