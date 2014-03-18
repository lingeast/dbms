#include "rm.h"
#include <stdio.h>
#include <cstring>
RM_IndexScanIterator::RM_IndexScanIterator() : ix_scan(NULL) {}

RM_IndexScanIterator::~RM_IndexScanIterator() {
	this->close();
}

RC RM_IndexScanIterator::close() {
	return this->ix_scan->close();
}

RC RM_IndexScanIterator::getNextEntry(RID& rid, void* key) {
	return this->ix_scan->getNextEntry(rid, key);
}

void RM_IndexScanIterator::set_itr(IX_ScanIterator* that_scan) {
	if (that_scan == NULL) {
		throw new std::logic_error("RM_IndexScanIterator need non-null ptr");
	} else {
		this->ix_scan = that_scan;
	}
}

RelationManager* RelationManager::_rm = 0;

RelationManager* RelationManager::instance()
{
    if(!_rm)
        _rm = new RelationManager();

    return _rm;
}

RelationManager::RelationManager() : TBL_T_SYSTEM("system"), rbfm(RecordBasedFileManager::instance())
{
	const int MAX_TABLE_NAME = 32;
	const int MAX_COL_NAME = 32;
	const int MAX_COL_TYPE = 32;
	Attribute attr;
	// Init tableRecord descriptor
	attr.name = "tableName";
	attr.type = TypeVarChar;
	attr.length = AttrLength(MAX_TABLE_NAME);
	tblRecord.push_back(attr);

	attr.name = "tableType";
	attr.type = TypeVarChar;
	attr.length = AttrLength(MAX_TABLE_NAME);
	tblRecord.push_back(attr);

	attr.name = "colNum";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	tblRecord.push_back(attr);


	// Init columnRecord descriptor
	attr.name = "tableName";
	attr.type = TypeVarChar;
	attr.length = AttrLength(MAX_TABLE_NAME);
	colRecord.push_back(attr);

	attr.name = "colName";
	attr.type = TypeVarChar;
	attr.length = AttrLength(MAX_COL_NAME);
	colRecord.push_back(attr);

	attr.name = "colType";
	attr.type = TypeVarChar;
	attr.length = AttrLength(MAX_COL_TYPE);
	colRecord.push_back(attr);

	attr.name = "colPosition";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	colRecord.push_back(attr);

	attr.name = "maxSize";
	attr.type = TypeInt;
	attr.length = (AttrLength)4;
	colRecord.push_back(attr);


}

RelationManager::~RelationManager()
{
}
const char TABLE_CATALOG[] = "table.tbl";
const char COL_CATALOG[] = "column.tbl";
const char TABLE_SUFFIX[] = ".db";

RC RelationManager::createTable(const string &tableName, const vector<Attribute> &attrs)
{
	//RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
	// Check if table already exists
	FileHandle tempF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tempF) == 0) {	// table file already exists
		rbfm->closeFile(tempF);
		return -1;
	}

	// Insert into table.tbl
	//cout << "Insert into table.tbl" << endl;
	FileHandle tableT;
	string defaultTable(TABLE_CATALOG);
	if (rbfm->openFile(defaultTable, tableT) != 0) {	// table catalog does not exist
		rbfm->createFile(defaultTable);	// try create table catalog
		rbfm->openFile(defaultTable, tableT);	// open newly created table catalog
	}
	if (tableT.getFile() == NULL) // can't open table catalog
		return -1;

	tableRecord tr(tableName, this->TBL_T_SYSTEM, attrs);
	RID tableID;
	//rbfm->printRecord(this->tblRecord, tr.recordData());
	if (rbfm->insertRecord(tableT, this->tblRecord, tr.recordData(), tableID) != 0) return -1;
	if (rbfm->closeFile(tableT) != 0) return -1;


	// Insert into column.tbl
	//cout << "Insert into column.tbl" << endl;
	FileHandle colT;
	string defaultCol(COL_CATALOG);
	if (rbfm->openFile(defaultCol, colT) != 0) {	// table catalog does not exist
		rbfm->createFile(defaultCol);	// try create table catalog
		rbfm->openFile(defaultCol, colT);	// open newly created table catalog
	}
	if (colT.getFile() == NULL)
		return -1;
	for (unsigned int i = 0; i < attrs.size(); i++) {
		RID recordID;
		columnRecord cr(tableName, attrs[i], i);
		//rbfm->printRecord(this->colRecord, cr.recordData());
		if (rbfm->insertRecord(colT, this->colRecord, cr.recordData(), recordID) != 0) return -1;
	}
	if(rbfm->closeFile(colT) != 0) return -1;

	return rbfm->createFile(tableName + TABLE_SUFFIX);
}

RC RelationManager::deleteTable(const string &tableName)
{
	// delete index of this table
	vector<Attribute> recDscptr;
	this->getAttributes(tableName, recDscptr);
	IndexManager *im = IndexManager::instance();
	for (int i = 0; i < recDscptr.size(); i++) {
		string idxName(indexName(tableName, recDscptr[i].name));
		FILE* fp = fopen(idxName.c_str(), "r");
		if (fp != NULL) { //index file exists, insert into index file
			fclose(fp);
			if (im->destroyFile(idxName) != 0) return -1;
		}
	}

	// delete table file
	if (rbfm->destroyFile(tableName + TABLE_SUFFIX) != 0) return -1;

	RM_ScanIterator RM_tblit;
	// Prepare condition string
	char* tblNameVal = new char[sizeof(int32_t) + tableName.size()];
	uint32_t nameLen = tableName.size();
	memcpy(tblNameVal, &nameLen, sizeof(nameLen));
	assert(sizeof(nameLen) == 4);
	memcpy(tblNameVal + sizeof(nameLen), tableName.c_str(), nameLen);

	vector<string> attributeNames;
	attributeNames.push_back(tblRecord[tblRecord.size() - 1].name);

// clean in table catalog
	scan(string(TABLE_CATALOG),	//tableName
	      string(tblRecord[0].name),	//conditionAttribute
	      EQ_OP,				// CompOp
	      tblNameVal,					// void* value
	      attributeNames,	// vector<string>& attributeNames
	      RM_tblit);			// rm_ScanIterator

	RID rid;
	uint32_t colNum;
	//char buffer[2048];
	int ret = RM_tblit.getNextTuple(rid, &colNum);
	//cout << "colNum: " << colNum << endl;
	RM_tblit.close();
	if (ret == RM_EOF) return -1;

	FileHandle tblF;
	rbfm->openFile(string(TABLE_CATALOG), tblF);
	if (rbfm->deleteRecord(tblF, vector<Attribute>(), rid)) return -1;
	rbfm->closeFile(tblF);

	// clean in column catalog
	vector<RID> colRID;
	RM_ScanIterator RM_colit;
	scan(string(COL_CATALOG),	//tableName
	      string(tblRecord[0].name),	//conditionAttribute
	      EQ_OP,				// CompOp
	      tblNameVal,					// void* value
	      vector<string>(),	// vector<string>& attributeNames
	      RM_colit);			// rm_ScanIterator
	RID tmprid;
	//cout << "Scan Column Table..." << endl;

	//uint32_t buffer;
	while(RM_colit.getNextTuple(tmprid, NULL) != EOF) {
		colRID.push_back(tmprid);
	}
	RM_colit.close();
	FileHandle colF;
	if (rbfm->openFile(string(COL_CATALOG), colF) != 0) return -1;
	for (int i = 0; i < colRID.size(); i++) {
		//cout << "RID: " << colRID[i].pageNum << " , " << colRID[i].slotNum << endl;
		if (rbfm->deleteRecord(colF, vector<Attribute>(), colRID[i]) != 0)
			return -1;
	}
	rbfm->closeFile(colF);

	delete tblNameVal;

    return 0;
}

int RelationManager::getTableColNum(const string &tableName) {
	//RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
		string tbllog(TABLE_CATALOG);
		FileHandle tbllogF;
		if (rbfm->openFile(tbllog, tbllogF) != 0) {
			return -1;
		} else {
			rbfm->closeFile(tbllogF);
		}
		/*
		const string &tableName,
		      const string &conditionAttribute,
		      const CompOp compOp,
		      const void *value,
		      const vector<string> &attributeNames,
		      RM_ScanIterator &rm_ScanIterator)
		      */
		RM_ScanIterator RM_tblit;
		vector<string> attributeNames;
		for (int i = 0; i < tblRecord.size(); i++)
			attributeNames.push_back(tblRecord[i].name);

		scan(string(TABLE_CATALOG),	//tableName
		      string("not used"),	//conditionAttribute
		      NO_OP,				// CompOp
		      NULL,					// void* value
		      attributeNames,	// vector<string>& attributeNames
		      RM_tblit);			// rm_ScanIterator

		char buffer[2000];
		RID id;
		int ret = 0;
		char* cursor = buffer;

		while((ret = RM_tblit.getNextTuple(id, buffer)) != RM_EOF) {
			uint32_t* tableNameLen = (uint32_t*)(buffer + 0);
			//cout << "tableNameSize" << *tableNameLen << endl;
			if (*tableNameLen == tableName.size()) {
				if (strncmp(tableName.c_str(), buffer + sizeof(int32_t), *tableNameLen) == 0)
				{
					cursor += sizeof(int32_t) + *tableNameLen;
					cursor += sizeof(int32_t) + *((uint32_t*)cursor);
					break;
				}
			}
		}

		RM_tblit.close();
		if (cursor == buffer) return -1;
		else return (int) *((uint32_t*)cursor);
}

int RelationManager::fillAttr(void* record, Attribute& attr) {
	char* cursor = (char*) record;
	cursor += sizeof(int32_t) + *((uint32_t*)cursor);	// skip first Varchar

	attr.name = string(cursor + sizeof(int32_t), *((uint32_t*)cursor));	// deal second Varchar
	//std::cout << "Attr Name: " << attr.name << std::endl;
	cursor += sizeof(int32_t) + *((uint32_t*)cursor);	// skip second Varchar


	switch(*(cursor + sizeof(int32_t))) {
	case 's': attr.type = TypeVarChar; break;
	case 'i': attr.type = TypeInt; break;
	case 'r': attr.type = TypeReal; break;
	default: assert(false);
	}
	cursor += sizeof(int32_t) + *((uint32_t*)cursor); // skip third Varchar
	int pos = *((uint32_t*)cursor);

	//std::cout << "colPosition " << pos << std::endl;
	cursor += sizeof(int32_t); // skip forth int

	attr.length = *((uint32_t*)cursor);
	//std::cout << "length = " << attr.length << std::endl;
	return pos;
}

string RelationManager::indexName(const string& tableName, const string& attrName) {
	return tableName + "_" + attrName + ".idx";
}

/*
 * return Attribute for given table name and attribute name
 * if no such attribute or table, return empty Attribute (Attribute.name is empty)
 */
Attribute RelationManager::getAttr(const string &tableName, const string &attributeName) {
	vector<Attribute> attributes;
	getAttributes(tableName, attributes);
	for (int i = 0; i < attributes.size(); i++) {
		if(attributes[i].name == attributeName) {
			return attributes[i];
		}
	}
	Attribute attr;
	return attr;
}

int RelationManager::fieldLen(void* data, const Attribute& attr) {
	//cout << "In RDU fieldLen" << endl;
	switch(attr.type) {
	case TypeInt:
		return attr.length;
	case TypeReal:
		return attr.length;
	case TypeVarChar:
		return *(uint32_t*)data + sizeof(uint32_t);
	default:
		assert(false);
	}

}

int RelationManager::getAttrOff(const void* record, const vector<Attribute> attrs, const string& attrName) {
	int len = 0;
	char* data = (char*) record;

	for (int i = 0; i < attrs.size(); i++) {
		if(attrs[i].name == attrName) return len;
		else {
			switch(attrs[i].type) {
			case TypeInt:
				len += attrs[i].length; break;
			case TypeReal:
				len += attrs[i].length; break;
			case TypeVarChar:
				len += *(uint32_t*)(data + len) + sizeof(uint32_t);
			}
		}
	}
	return -1; //indicating match failure
}

int RelationManager::recordMaxLen(const vector<Attribute>& attrs) {
	int max_len = 0;
	for (int i = 0; i < attrs.size(); i++) {
		max_len += attrs[i].length;
	}
	return max_len;
}

RC RelationManager::getAttributes(const string &tableName, vector<Attribute> &attrs)
{
	int attrSize = getTableColNum(tableName);
	//cout << "Table "<< tableName << "colNum: "<< attrSize << endl;
	if (attrSize <= 0) return -1;

	//cout << "Found matching table name in catalog, NUMCOL = " << attrSize << endl;
	//rbfm->printRecord(this->tblRecord, buffer);

	// Init vector of attribute with emptyAttr on each cell
	Attribute emptyAttr;
	emptyAttr.name = string("");
	emptyAttr.type = TypeInt;
	emptyAttr.length = 0;
	vector<Attribute> copyAttrs(attrSize, emptyAttr);


	vector<string> attributeNames;
	for (int i = 0; i < colRecord.size(); i++) {
		attributeNames.push_back(colRecord[i].name);
	}


	RM_ScanIterator RM_colit;

	scan(string(COL_CATALOG),	//tableName
	      string("not used"),	//conditionAttribute
	      NO_OP,				// CompOp
	      NULL,					// void* value
	      attributeNames,	// vector<string>& attributeNames
	      RM_colit);			// rm_ScanIterator

	// Found matching table name in catalog
	int cnt = 0;
	char buffer[2048];
	RID id;
	while(cnt < attrSize && RM_colit.getNextTuple(id, buffer) != EOF) {
		uint32_t* tableNameLen = (uint32_t*)(buffer + 0);
		if (*tableNameLen == tableName.size()) {
			if (strncmp(tableName.c_str(),
				buffer + sizeof(uint32_t),
				*tableNameLen) == 0) {
				// find Matching, print out
				//std::cout << "I FOUND IT, ID = ("<< id.pageNum << "," << id.slotNum << ")" << std::endl;
				//rbfm->printRecord(colRecord, buffer);
				Attribute attr;
				int pos = fillAttr(buffer, attr);
				copyAttrs[pos] = attr;
				++cnt;
			}
		}

	}
	if (RM_colit.close() != 0) return false;
	attrs = copyAttrs;
	/*
    for(unsigned i = 0; i < attrs.size(); i++)
    {
        cout << "Attribute Name: " << attrs[i].name << endl;
        cout << "Attribute Type: " << (AttrType)attrs[i].type << endl;
        cout << "Attribute Length: " << attrs[i].length << endl << endl;
    }
    */
	return 0;
}

RC RelationManager::insertTuple(const string &tableName, const void *data, RID &rid)
{
	FileHandle tableF;

	vector<Attribute> recDscptr;
	if (getAttributes(tableName, recDscptr) != 0){
		rbfm->closeFile(tableF);
		return -1;
	}

	//cout << "Insert into " << tableName << endl;
	// insert into table
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	if (rbfm->insertRecord(tableF, recDscptr, data, rid) != 0) {
		rbfm->closeFile(tableF);
		return -1;
	}
	if (rbfm->closeFile(tableF) != 0) return -1;

	// insert into index
	IndexManager *im = IndexManager::instance();
	char *recData = (char*) data;
	int len = 0;
	for (int i = 0; i < recDscptr.size(); i++) {
		string idxName(indexName(tableName, recDscptr[i].name));
		FILE* fp = fopen(idxName.c_str(), "r");
		if (fp != NULL) { //index file exists, insert into index file
			fclose(fp);
			FileHandle idxF;
			if (im->openFile(idxName, idxF) != 0)
				return -1;
			if (im->insertEntry(idxF, recDscptr[i], recData + len, rid) != 0) {
				im->closeFile(idxF);
				return -1;
			}
			if (im->closeFile(idxF) != 0)
				return -1;
		}
		len += fieldLen(recData + len, recDscptr[i]);
	}
	return 0;
}

RC RelationManager::deleteTuples(const string &tableName)
{
	// update table file
	if (rbfm->destroyFile(tableName + TABLE_SUFFIX) != 0) return -1;
	if (rbfm->createFile(tableName + TABLE_SUFFIX) != 0) return -1;

	// update relative index file
	vector<Attribute> recDscptr;
	this->getAttributes(tableName, recDscptr);
	for (int i = 0; i < recDscptr.size(); i++) {
		string idxName(indexName(tableName, recDscptr[i].name));
		FILE* fp = fopen(idxName.c_str(), "r");
		if (fp != NULL) { //index file exists, insert into index file
			fclose(fp);
			if (this->destroyIndex(tableName, recDscptr[i].name) != 0) return -1;
			if (this->createIndex(tableName, recDscptr[i].name) != 0) return -1;
		}
	}
	return 0;

}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
	FileHandle tableF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	//vector<Attribute> recDscptr; // Not used!

	vector<Attribute> recDscptr;
	if (getAttributes(tableName, recDscptr) != 0){
		rbfm->closeFile(tableF);
		return -1;
	}
	// retrieve record first (used for deleting record in index)

	char recData[recordMaxLen(recDscptr)]; // why this line works?
	if (rbfm->readRecord(tableF, recDscptr, rid, recData) != 0) return -1;


	// delete record at table
	if (rbfm->deleteRecord(tableF, vector<Attribute>(), rid) != 0) return -1;
	if (rbfm->closeFile(tableF) != 0) return -1;


	// << "Try delete at index" << endl;
	// delete attribute  at index
	IndexManager *im = IndexManager::instance();
	int len = 0;
	for (int i = 0; i < recDscptr.size(); i++) {
		string idxName(indexName(tableName, recDscptr[i].name));
		FILE* fp = fopen(idxName.c_str(), "r");
		if (fp != NULL) { //index file exists, insert into index file
			fclose(fp);
			FileHandle idxF;
			if (im->openFile(idxName, idxF) != 0)
				return -1;
			if (im->deleteEntry(idxF, recDscptr[i], (void*)(recData + len), rid) != 0)
				return -1;
			if (im->closeFile(idxF) != 0)
				return -1;
		}
		len += fieldLen(recData + len, recDscptr[i]);
	}

	return 0;
}

RC RelationManager::updateTuple(const string &tableName, const void *data, const RID &rid)
{
	FileHandle tableF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	vector<Attribute> recDscptr;
	if (getAttributes(tableName, recDscptr) != 0) {
		rbfm->closeFile(tableF);
		return -1;
	}

	char oldData[this->recordMaxLen(recDscptr)];
	if (rbfm->readRecord(tableF, recDscptr, rid, oldData) != 0) return -1;

	// update into index
	IndexManager *im = IndexManager::instance();
	char *newData = (char*) data;
	int newLen = 0, oldLen = 0;
	for (int i = 0; i < recDscptr.size(); i++) {
		string idxName(indexName(tableName, recDscptr[i].name));
		FILE* fp = fopen(idxName.c_str(), "r");
		if (fp != NULL) { //index file exists, insert into index file
			fclose(fp);
			FileHandle idxF;
			if (im->openFile(idxName, idxF) != 0)
				return -1;
			// remove old
			if (im->deleteEntry(idxF, recDscptr[i], oldData + oldLen, rid) != 0) {
				im->closeFile(idxF); return -1;
			}
			// insert new
			if (im->insertEntry(idxF, recDscptr[i], newData + newLen, rid) != 0) {
				im->closeFile(idxF); return -1;
			}
			if (im->closeFile(idxF) != 0)
				return -1;
		}
		newLen += fieldLen(newData + newLen, recDscptr[i]);
		oldLen += fieldLen(oldData + oldLen, recDscptr[i]);
	}

	if (rbfm->updateRecord(tableF, recDscptr, data, rid) != 0) return -1;
    if (rbfm->closeFile(tableF) != 0) return -1;

    return 0;
}

RC RelationManager::readTuple(const string &tableName, const RID &rid, void *data)
{
	FileHandle tableF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	vector<Attribute> recDscptr;
	//cout << "Want to get attri" << endl;
	if (getAttributes(tableName, recDscptr) != 0){
		rbfm->closeFile(tableF);
		return -1;
	}

	int ret =  rbfm->readRecord(tableF, recDscptr,  rid, data);
	//std::cout << "readRecord End" << std::endl;
	if (rbfm->closeFile(tableF) !=  0) return -1;
	return ret;
}

RC RelationManager::readAttribute(const string &tableName, const RID &rid, const string &attributeName, void *data)
{
	FileHandle tableF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	vector<Attribute> recDscptr;
	if (getAttributes(tableName, recDscptr) != 0) {
		rbfm->closeFile(tableF);
		return -1;
	}
	char middle_data[2048];
	int ret = rbfm->readRecord(tableF, recDscptr, rid, middle_data);
	if (rbfm->closeFile(tableF) != 0 || ret != 0) return -1;
	char* cursor = middle_data + 0;
	for (int i = 0; i < recDscptr.size(); i++) {
		if (attributeName.compare(recDscptr.at(i).name) != 0) {	// not match, move cursor to next
			switch (recDscptr.at(i).type) {
			case TypeVarChar:
				cursor += sizeof(int32_t) + *((uint32_t*)cursor);
				break;
			case TypeInt:
				cursor += sizeof(int32_t);
				break;
			case TypeReal:
				cursor += sizeof(int32_t);
				break;
			default:
				assert(false);
				break;
			}
		} else {	//match
			switch (recDscptr.at(i).type) {
			case TypeVarChar:
				memcpy(data, cursor, sizeof(int32_t) + *((uint32_t*)cursor));
				break;
			case TypeInt:
				memcpy(data, cursor, sizeof(int32_t));
				break;
			case TypeReal:
				memcpy(data, cursor, sizeof(int32_t));
				break;
			default:
				assert(false);
				break;
			}
			return 0;
		}
	}
	return -1;	// no matching reach end
}

RC RelationManager::reorganizePage(const string &tableName, const unsigned pageNumber)
{
	FileHandle tableF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	vector<Attribute> recDscptr;
	if (getAttributes(tableName, recDscptr) != 0){
		rbfm->closeFile(tableF);
		return -1;
	}
	int ret = rbfm->reorganizePage(tableF, recDscptr, pageNumber);
	if (rbfm->closeFile(tableF) != 0) return -1;
	return ret;
}

RC RelationManager::scan(const string &tableName,
      const string &conditionAttribute,
      const CompOp compOp,                  
      const void *value,                    
      const vector<string> &attributeNames,
      RM_ScanIterator &rm_ScanIterator)
{
	vector <Attribute> recDscptr;
    if (tableName.compare(TABLE_CATALOG) == 0) {
    	recDscptr = tblRecord;
    	FileHandle fh;
    	//RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
    	if (rbfm->openFile(tableName, fh) != 0){
    		cout << "Open TABLE_CATALOG failed" << endl;
    		return -1;
    	}
    	rm_ScanIterator.setIterator(fh, recDscptr, conditionAttribute,
    			compOp, value, attributeNames, rbfm);

    	//return rbfm->closeFile(fh);
    	return 0;
    }
    if (tableName.compare(COL_CATALOG) == 0) {
    	recDscptr = colRecord;
    	FileHandle fh;
    	//RecordBasedFileManager* rbfm = RecordBasedFileManager::instance();
    	if (rbfm->openFile(tableName, fh) != 0) return -1;
    	rm_ScanIterator.setIterator(fh, recDscptr, conditionAttribute,
    	    			compOp, value, attributeNames, rbfm);

    	//return rbfm->closeFile(fh);
    	return 0;
    }

    if (getAttributes(tableName, recDscptr) != 0) return -1;
    FileHandle tblF;
    if (rbfm->openFile(tableName + TABLE_SUFFIX, tblF) != 0) return -1;
    rm_ScanIterator.setIterator(tblF, recDscptr, conditionAttribute,
    		compOp, value, attributeNames, rbfm);

    //assert(false);
    return 0;
}

// Extra credit
RC RelationManager::dropAttribute(const string &tableName, const string &attributeName)
{
    return -1;
}

// Extra credit
RC RelationManager::addAttribute(const string &tableName, const Attribute &attr)
{
    return -1;
}

// Extra credit
RC RelationManager::reorganizeTable(const string &tableName)
{
    return -1;
}


RC RelationManager::createIndex(const string& tableName, const string& attributeName) {
	// Find attribute
	Attribute attr = this->getAttr(tableName, attributeName);
	if (attr.name.empty()) // get attribute information failed
		return -1;
	//cout << "find attr in the catalog: (" << attr.name<<"," << attr.length<<")";

	// Scan table to insert attribute into index file one by one
	RM_ScanIterator RM_itr;
	vector<string> attributeNames;
	attributeNames.push_back(attributeName);
	this->scan(tableName,	//tableName
	      string("not used"),	//conditionAttribute
	      NO_OP,				// CompOp
	      NULL,					// void* value
	      attributeNames,	// vector<string>& attributeNames
	      RM_itr);			// rm_ScanIterator

	IndexManager* im = IndexManager::instance();
	string idxName = indexName(tableName, attributeName);
	if (im->createFile(idxName) != 0) return -1;
	FileHandle idxFH;
	if (im->openFile(idxName, idxFH) != 0) return -1;

	RID rid;
	char buffer[attr.length];
	int time1 = 0;
	//cout << "In create CreateIndex " << tableName << " , "<<attributeName<< endl;

	while(RM_itr.getNextTuple(rid, buffer) != RM_EOF) {
		im->insertEntry(idxFH, attr, buffer, rid);
		//cout << "Insert " << ++time1 << ", Value = " << *(float*)buffer << endl;
	}

	im->closeFile(idxFH);
	//cout << "After create CreateIndex" << endl;
	//TODO remove test code

	/*
	IX_ScanIterator ix_itr;
	im->scan(idxFH, attr, NULL, NULL, true, true, ix_itr);
	int time = 0;
	while(ix_itr.getNextEntry(rid, buffer) == 0) {
		cout << "time:" << ++time << endl;
	}
	 */
	//TODO test code end


	return 0;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName) {
	IndexManager* im = IndexManager::instance();
	return im->destroyFile(this->indexName(tableName, attributeName));
}

 // indexScan returns an iterator to allow the caller to go through qualified entries in index
 RC RelationManager::indexScan(const string &tableName,
                       const string &attributeName,
                       const void *lowKey,
                       const void *highKey,
                       bool lowKeyInclusive,
                       bool highKeyInclusive,
                       RM_IndexScanIterator &rm_IndexScanIterator)
 {
	 IndexManager* im = IndexManager::instance();
	 FileHandle indexF;


	 if (im->openFile(indexName(tableName, attributeName), indexF) != 0)
		 return -1;	// failed on index file

	 Attribute attr = this->getAttr(tableName, attributeName);
	 if (attr.name.empty()) return -1;	// failed retrieve attribute type
	 //else cout << attr.name << "==" << attributeName << endl;

	 //cout << "=======In RM::indexScan, call IM::scan========" << endl;

	 IX_ScanIterator* ix_scan_ptr = new IX_ScanIterator();
	 int ret = IndexManager::instance()->scan(indexF, attr,
			 lowKey, highKey, lowKeyInclusive, highKeyInclusive, *ix_scan_ptr);

	 if (ret != 0) return -1;
	 //cout << "=======In RM::indexScan, call IM::scan END========" << endl;

	 /*
	 char buffer[2000];
	 RID rid;
	 int time = 0;
	 while(ix_scan_ptr->getNextEntry(rid, buffer) != -1) {
		 cout << "IX_SCAN_ITR test: " << ++time << endl;
	 }
	 */

	 rm_IndexScanIterator.set_itr(ix_scan_ptr);

	 return 0;

 }
