#include "rm.h"
/*
RC RM_ScanIterator::loadConfig(RecordBasedFileManager* sing_rbfm,
			const vector <Attribute>& recordDescriptor,
			const FileHandle& fileH) {
	return RBDM_it.loadConfig(sing_rbfm, recordDescriptor, fileH);
}
*/
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
	// TODO : remove test code
/*
	RM_ScanIterator RM_it;
	scan(defaultCol,
	      string("not used"),
	      EQ_OP,
	      NULL,
	      vector<string>(),
	      RM_it);

	char rawData[2000];
	RID id;
	std::cout << "Iterator Begin" << std::endl;
	while(RM_it.getNextTuple(id, rawData) != RM_EOF) {
		std::cout << "PageNum: " << id.pageNum << " SlotNum: " << id.slotNum << std::endl;
		rbfm->printRecord(colRecord, rawData);
	}

	RM_it.close();
	*/
	// TODO : test code end
	if(rbfm->closeFile(colT) != 0) return -1;

	return rbfm->createFile(tableName + TABLE_SUFFIX);
}

RC RelationManager::deleteTable(const string &tableName)
{

	RM_ScanIterator RM_tblit;
	// Prepare condition string
	char* tblNameVal = new char[sizeof(int32_t) + tableName.size()];
	uint32_t nameLen = tableName.size();
	memcpy(tblNameVal, &nameLen, sizeof(nameLen));
	assert(sizeof(nameLen) == 4);
	memcpy(tblNameVal + sizeof(nameLen), tableName.c_str(), nameLen);

	//cout << "StrLen: " << *((int32_t*) tblNameVal) << endl;

	//for (int i = 0; i < 4 + tableName.size(); i++) {
	//	cout <<  *(tblNameVal + i) << " ," << endl;
	//}

	vector<string> attributeNames;


	//for (int i = 0; i < tblRecord.size(); i++) {
		//attributeNames.push_back(tblRecord[i].name);
	//}

	attributeNames.push_back(tblRecord[tblRecord.size() - 1].name);


	//cout << "Condition Attribute: " ;
	//for (int i = 0; i < attributeNames.size(); i++) {
	//	cout << attributeNames[i];
	//}

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

	FileHandle colF;
	if (rbfm->openFile(string(COL_CATALOG), colF) != 0) return -1;
	for (int i = 0; i < colRID.size(); i++) {
		//cout << "RID: " << colRID[i].pageNum << " , " << colRID[i].slotNum << endl;
		if (rbfm->deleteRecord(colF, vector<Attribute>(), colRID[i]) != 0)
			return -1;
	}

	delete tblNameVal;
	//rbfm->printRecord(this->tblRecord, buffer);
	//cout << "ColNum: " << *((uint32_t*)buffer) << endl;
	//out << "ColNum: " << colNum << endl;
	//cout << "RID: " << rid.pageNum << " , " << rid.slotNum << endl;

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
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	vector<Attribute> recDscptr;
	if (getAttributes(tableName, recDscptr) != 0){
		rbfm->closeFile(tableF);
		return -1;
	}
	int ret =  rbfm->insertRecord(tableF, recDscptr, data, rid);
	if (rbfm->closeFile(tableF) != 0) return -1;
	return ret;
}

RC RelationManager::deleteTuples(const string &tableName)
{
	if (rbfm->destroyFile(tableName + TABLE_SUFFIX) != 0) return -1;

	return rbfm->createFile(tableName + TABLE_SUFFIX);
}

RC RelationManager::deleteTuple(const string &tableName, const RID &rid)
{
	FileHandle tableF;
	if (rbfm->openFile(tableName + TABLE_SUFFIX, tableF) != 0) return -1;
	//vector<Attribute> recDscptr; // Not used!

	int ret = rbfm->deleteRecord(tableF, vector<Attribute>(), rid);

	if (rbfm->closeFile(tableF) != 0) return -1;
	return ret;
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
	int ret = rbfm->updateRecord(tableF, recDscptr, data, rid);
    if (rbfm->closeFile(tableF) != 0) return -1;
    return ret;
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
	//cout << "get attr end" << endl;
	// TODO remove test code
	//std::cout << "readRecord Begin" << std::endl;
	// TODO test code end
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
	RM_ScanIterator RM_table_itr;
	vector<string> attributeNames;
	attributeNames.push_back(attributeName);

	// Prepare tableName data
	char* tblNameVal = new char[sizeof(int32_t) + tableName.size()];
	uint32_t nameLen = tableName.size();
	memcpy(tblNameVal, &nameLen, sizeof(nameLen));
	memcpy(tblNameVal + sizeof(nameLen), tableName.c_str(), nameLen);

	scan(string(TABLE_CATALOG),	//tableName
	      tableName,	//conditionAttribute
	      EQ_OP,				// CompOp
	      tblNameVal,					// void* value
	      attributeNames,	// vector<string>& attributeNames
	      RM_table_itr);			// rm_ScanIterator
	return -1;
}

RC RelationManager::destroyIndex(const string &tableName, const string &attributeName) {
	return -1;
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
	 return -1;
 }
