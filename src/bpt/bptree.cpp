/*
 * bptree.cpp
 *
 *  Created on: Feb 25, 2014
 *      Author: yidong
 */

#include "bptree.h"
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <cassert>

using namespace std;
namespace BPlusTree{

bp_tree::bp_tree(const char *name) : fName(name), file(NULL), fhelp(NULL), key_itr(NULL), RCinsert(0) {
	file = fopen(name, "r+b");
	if (file == NULL) {
		// Creating new btree file
		file = fopen(name, "w+b");
		if (file == NULL)
				throw new std::runtime_error("Can not open/create B+ tree file");

		// Add initial directory to file
		fhelp = new file_helper(file);
		// this->dir has been initialized by default constructor
		fhelp->write_page(0, dir.page_block());
	} else {
		fhelp = new file_helper(file);
		// Read existing directory page out
		fhelp->read_page(0, dir.page_block());
	}
}

bp_tree::bp_tree(const bp_tree& that) : fName(that.fName.c_str()), file(NULL), fhelp(NULL), key_itr(NULL), RCinsert(0) {
	file = fopen(fName.c_str(), "r+b");
	if (file == NULL) {
		// Creating new btree file
		file = fopen(fName.c_str(), "w+b");
		if (file == NULL)
				throw new std::runtime_error("Can not open/create B+ tree file");

		// Add initial directory to file
		fhelp = new file_helper(file);
		// this->dir has been initialized by default constructor
		fhelp->write_page(0, dir.page_block());
	} else {
		fhelp = new file_helper(file);
		// Read existing directory page out
		fhelp->read_page(0, dir.page_block());
	}
}

bp_tree::~bp_tree() {
	if (file != NULL) fclose(file);
	if (fhelp != NULL) delete fhelp;
	if (key_itr != NULL) delete key_itr;
}

int bp_tree::delete_entry(bt_key *key, RID rid) {
	if (key_itr == NULL) {
		key_itr = key->clone();
	}
	int ret = -1;
	if (dir.root() < 1){
		return -1;
	}
	page_node pg(dir.root());
	fhelp->read_page(pg.page_id(), pg.page_block());

	while(!pg.is_leaf_node()){
		int entrypg = pg.findEntry(key,key_itr);
		pg.set_id(entrypg);
		fhelp->read_page(pg.page_id(),pg.page_block());
	}


	ret = pg.delete_leaf(key,rid,key_itr);
	fhelp ->write_page(pg.page_id(),pg.page_block());
	return ret;

}

int bp_tree::insert_entry(bt_key *key, RID rid) {
	if (key_itr == NULL) {
		key_itr = key->clone();
	}

	if (dir.root() < 1) {
		// empty btree
		cout << "Empty Bpt File" << endl;
		page_node root(Leaf, 1, 0, 0);
		//dir[1] = 1;
		fhelp->write_page(dir.find_empty(), root.page_block());
		dir.update_root(root.page_id());
		fhelp->write_page(0, dir.page_block());
	}

	//cout << "Root = " << dir.root() << endl;
	page_node root(dir.root());
	fhelp->read_page(root.page_id(), root.page_block());
	key = insert_to_page(root, key, rid);
	if (RCinsert != 0){
		RCinsert = 0;
		return -1;
	}
	if (key != NULL){
		uint16_t splitpage = dir.find_empty();
		/*
		for (uint16_t i=1;i<PAGE_SIZE/sizeof(uint16_t);i++){
			if(dir[i]==0){
				splitpage = i;
				break;
			}
		}
		*/
		assert(splitpage != 0);
		//cout << "Split at root, SplitPage = " << splitpage << endl;

		page_node newroot(Index, splitpage, 0, 0);
		dir.update_root(newroot.page_id());
		//dir[splitpage] = splitpage;
		fhelp->write_page(0, dir.page_block());

		// fill in new root page
		uint16_t id = root.page_id();
		memcpy(newroot.content_block(), &id, sizeof(int16_t));
		memcpy(newroot.content_block() + sizeof(root.page_id()), key->data(), key->length());
		memcpy(newroot.content_block() + sizeof(int16_t) + key->length(), &root.right_id(), sizeof(int16_t));
		newroot.end_offset() = key->length() + 2 * sizeof(int16_t);
		fhelp->write_page(newroot.page_id(), newroot.page_block());
	} else {
		//cout << "No Update in root" << endl;
	}
	//cout << "Insert succesfully" << endl;
	return 0;
	//insert_to_page
}

bt_key* bp_tree::insert_to_page(page_node& pg, bt_key* key, RID rid) {
	if (pg.is_leaf_node()) {
		if (pg.check_duplicate(key,rid,key_itr) != 0) {
			RCinsert = -1;
			return NULL;
		}
		if (key->length() + sizeof(rid) <= PAGE_SIZE - pg.end_offset() - sizeof(int16_t) * 4){
			//cout << "Enough Room in Leaf" << endl;
			pg.insert(key,rid,key_itr);

			//cout << "Write to page" << pg.page_id() << endl;
			fhelp -> write_page(pg.page_id(),pg.page_block());
			return NULL;
		}else{
			uint16_t splitpage = dir.find_empty();
			/*
			for (uint16_t i=1;i<PAGE_SIZE/sizeof(uint16_t);i++){
				if(dir[i]==0){
					splitpage = i;
					break;
				}
			}

			if (splitpage == 0) {
				throw new std::runtime_error("Not enough room in dir page!");
			}
			dir[splitpage] = splitpage;
			*/
			//cout << "Not Enough Room in Leaf" << endl;
			//cout << "SplitPage = " << splitpage << endl;
			page_node splitpg(Leaf, splitpage, pg.page_id(), pg.right_id());
			pg.right_id() = splitpage;
			int flag = 1,splitpos = 0;

			splitpos = pg.findHalf(key,rid,flag,key_itr);

			memcpy(splitpg.content_block(), pg.content_block() + splitpos, pg.end_offset() - splitpos);
			splitpg.end_offset() = pg.end_offset() - splitpos; // Update new page end_off
			pg.end_offset() = splitpos;	// Update old page end_off


			if (flag == 0)
				pg.insert(key,rid,key_itr);
			else splitpg.insert(key,rid,key_itr);

			bt_key* newkey = key;
			newkey -> load(splitpg.content_block());
			fhelp -> write_page(0,dir.page_block());

			assert(pg.page_id() == pg.page_id());
			fhelp -> write_page(pg.page_id(),pg.page_block());

			assert(splitpg.page_id() == splitpg.page_id());
			fhelp -> write_page(splitpg.page_id(),splitpg.page_block());

			return newkey;
		}
		// insert into leaf node
		// if splitting happen return a key ptr
		// else return NULL ptr
	} else {
		// Page is Index Node, find next step
		int test = pg.findEntry(key, key_itr);
		page_node child_pg(test);
		fhelp->read_page(child_pg.page_id(),child_pg.page_block());
		key = insert_to_page(child_pg,key,rid);
		if (key != NULL){
			if(key->length() + sizeof(int16_t) <= PAGE_SIZE - pg.end_offset() - sizeof(int16_t) * 4){
				//cout << "Enough Room in Index" << endl;
				rid.pageNum = child_pg.right_id();
				pg.insert(key,rid,key_itr);
				fhelp -> write_page(pg.page_id(), pg.page_block());
				return NULL;
			}
			else{
				//cout << "Not Enough Room in Index" << endl;
				uint16_t splitpage = dir.find_empty();
				/*
				for (uint16_t i=1;i<PAGE_SIZE/sizeof(uint16_t);i++){
					if(dir[i]==0){
						splitpage = i;
						break;
					}
				}

				dir[splitpage] = splitpage;
				*/
				//cout << "SplitPage = " << splitpage << endl;
				page_node splitpg(Index,splitpage,pg.page_id(),pg.right_id());
				pg.right_id() = splitpage;
				int flag = 1,splitpos = 0;
				splitpos = pg.findHalf(key,rid,flag,key_itr);
				if(flag == 0){
					key_itr->load(pg.content_block() + splitpos);
					memcpy(splitpg.content_block(),pg.content_block() + splitpos + key_itr->length() ,pg.end_offset() - splitpos - key_itr->length());
					splitpg.end_offset() = pg.end_offset() - splitpos -  key_itr->length(); // update new end of right leaf;
					pg.right_id() = splitpg.page_id(); // update right id of original leaf
					pg.end_offset() = splitpos; // update new end offset of orginal leaf
					void *tempkey = malloc(key_itr->length());
					memcpy(tempkey,key_itr->data(),key_itr->length());
					rid.pageNum = child_pg.right_id();
					pg.insert(key,rid,key_itr);
					key->load(tempkey);
					free(tempkey);

					fhelp ->write_page(0,dir.page_block());
					fhelp ->write_page(pg.page_id(),pg.page_block());
					fhelp ->write_page(splitpg.page_id(),splitpg.page_block());

					return key;
				}
				else {
					key_itr-> load(pg.content_block() + splitpos);
					if(*key < *key_itr){
						page_node childpg(*(int16_t*)(pg.content_block() + splitpos - sizeof(int16_t)));
						fhelp->read_page(childpg.page_id(), childpg.page_block());
						memcpy(splitpg.content_block(), &childpg.right_id(), sizeof(int16_t));
						memcpy(splitpg.content_block() + sizeof(int16_t), pg.content_block() + splitpos, pg.end_offset() - splitpos);
						splitpg.end_offset() = pg.end_offset() - splitpos + sizeof(int16_t);
						pg.end_offset() = splitpos;
						pg.right_id() = splitpg.page_id();

						fhelp ->write_page(0,dir.page_block());
						fhelp ->write_page(pg.page_id(),pg.page_block());
						fhelp ->write_page(splitpg.page_id(),splitpg.page_block());
						return key;
					}else{
						key_itr->load(pg.content_block() + splitpos);
						memcpy(splitpg.content_block(),pg.content_block() + splitpos + key_itr->length(),pg.end_offset() - splitpos - key_itr->length());
						splitpg.end_offset() = pg.end_offset() - splitpos - key_itr->length();
						pg.right_id() = splitpg.page_id();
						pg.end_offset() = splitpos;
						rid.pageNum = child_pg.right_id();
						splitpg.insert(key,rid,key_itr);
						key->load(pg.content_block() + splitpos);

						fhelp ->write_page(0,dir.page_block());
						fhelp ->write_page(pg.page_id(),pg.page_block());
						fhelp ->write_page(splitpg.page_id(),splitpg.page_block());
						return key;
					}
				}
				fhelp ->write_page(0,dir.page_block());
				fhelp ->write_page(pg.page_id(),pg.page_block());
				fhelp ->write_page(splitpg.page_id(),splitpg.page_block());
			}
		} else {
			//cout << "Index ("<< pg.page_id() << ") does not get new push-up key" << endl;
			return NULL;
		}
		// find next page, read it out
		// insert_to_page(child_pg, key, rid);
		// if splitting happen return a key ptr
		// else return NULL ptr
	}
	return NULL;
}

}
