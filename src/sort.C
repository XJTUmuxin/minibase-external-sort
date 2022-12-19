// file sort.C  --  implementation of Sort class.

#include "sort.h"
#include "heapfile.h"
#include "system_defs.h"
#include <cstring>

#define SORTDEBUG 0
int offset = 0;
TupleOrder Sort_order;
int field_len = 0;


// Add you error message here
// static const char *sortErrMsgs[] {
// };
// static error_string_table sortTable(SORT, sortErrMsgs);

// constructor.
Sort::Sort ( char* inFile, char* outFile, int len_in, AttrType in[],
	     short str_sizes[], int fld_no, TupleOrder sort_order,
	     int amt_of_buf, Status& s )
{
	rec_len = 0;
	scans = new Scan*[amt_of_buf-1];
	amt_of_bytes = PAGESIZE*amt_of_buf;
	mem = new char[amt_of_bytes];
	if(SORTDEBUG){
		cout<<amt_of_bytes<<endl;
	}
	this->amt_of_buf = amt_of_buf;
	field_len = str_sizes[fld_no];
	Sort_order = sort_order;
	for(int i=0;i<len_in;++i){
		rec_len += sizeof(char)*str_sizes[i];
	}
	for(int i=0;i<fld_no;++i){
		offset += str_sizes[i];
	}
	if(firstPass(inFile,amt_of_buf,tempHFnum)==DONE){
		s = OK;
		return;
	}
	if(SORTDEBUG)cout<<tempHFs.size()<<endl;
	passnum = 2;
	while(tempHFs.size()!=1){
		if(SORTDEBUG)cout<<tempHFs.size()<<endl;
		followingPass(passnum);
		passnum++;
	}
	assert(tempHFs.size()==1);
	HeapFile hf1(tempHFs.front().data(),s);
	assert(s==OK);
	Scan* scan = hf1.openScan(s);
	assert(s==OK);
	HeapFile hf2(outFile,s);
	assert(s==OK);
	char* record = new char[rec_len];
	RID rid;
	int len;
	while(scan->getNext(rid,record,len)!=DONE){
		assert(hf2.insertRecord(record,len,rid)==OK);
	}
	hf1.deleteFile();
	delete[] record;
	s = OK;
}

Sort::~Sort(){
	offset = 0;
	delete[] mem;
	delete[] scans;
}

void Sort::makeHFname(char *&name, int passNum, int HFnum){
	name = new char[10]
	{'0','0','0','0','0','.','t','e','m','p'};// 2位passNum 3位HFnum 剩余 .temp
	string passno = to_string(passNum);
	string HFno = to_string(HFnum);
	for(int i=0;i<passno.size();++i){
		name[i+2-passno.size()] = passno[i];
	}
	for(int i=0;i<HFno.size();++i){
		name[i+5-HFno.size()] = HFno[i];
	}
	if(SORTDEBUG)cout<<name<<endl;
}

int tupleCmp(const void *pRec1, const void *pRec2) 
{
  int result;

  char *rec1 = (char *)pRec1;
  char *rec2 = (char *)pRec2;
  
  result = (strncmp(&rec1[offset], &rec2[offset], field_len));
	
  if (result < 0)
	if (Sort_order == Ascending)
	  return -1;
	else
	  return 1;
  else 
	if (result > 0)
	  if (Sort_order == Ascending)
		return 1;
	  else 
		return -1;
	else 
	  return 0;
}

Status Sort::firstPass(char *inFile, int bufferNum,int& tempHFnum){
	if(SORTDEBUG)cout<<"fistPass begin"<<endl;
	Status s;
	RID rid;
	HeapFile f(inFile,s);
	assert(s == OK);
	Scan *scan = f.openScan(s);
	assert(s == OK);
	char* temp = mem;
	int len;
	int record_num = 0;
	while(scan->getNext(rid,temp,len)!=DONE){
		record_num++;
		temp += len;
		if(record_num*len+len>amt_of_bytes){
			//排序
			qsort(mem,record_num,len,tupleCmp);
			char* name;
			makeHFname(name,1,tempHFnum);
			HeapFile f_temp(name,s);
			char* temp2 = mem;
			for(int i=0;i<record_num;++i){
				f_temp.insertRecord(temp2,len,rid);
				temp2 += len;
			}
			if(SORTDEBUG)cout<<"recordnum:"<<f_temp.getRecCnt()<<endl;
			tempHFs.emplace(name);
			delete[] name;
			tempHFnum++;
			record_num = 0;
			temp = mem;
		}
	}
	if(record_num>0){
		//排序
			qsort(mem,record_num,len,tupleCmp);
			//创建新的HF
			char* name;
			makeHFname(name,1,tempHFnum);
			HeapFile f_temp(name,s);
			char* temp2 = mem;
			for(int i=0;i<record_num;++i){
				f_temp.insertRecord(temp2,len,rid);
				temp2 += len;
			}
			tempHFs.emplace(name);
			delete[] name;
			tempHFnum++;
	}
	if(tempHFnum==0)return DONE;
	if(SORTDEBUG)cout<<"firstPass end"<<endl;
	return OK;
}

Status Sort::followingPass(int passNum){
	if(SORTDEBUG)cout<<"pass "<<passNum<<" begin"<<endl;
	int que_size = tempHFs.size();
	int scan_num;
	Status s;
	while(que_size){
		if(que_size<amt_of_buf-1){
			scan_num = que_size;
		}
		else{
			scan_num = amt_of_buf-1;
		}
		que_size -= scan_num;
		HeapFile** hf = new HeapFile*[scan_num];
		for(int i=0;i<scan_num;++i){
			string name = tempHFs.front();
			tempHFs.pop();
			hf[i] = new HeapFile(name.data(),s);
			assert(s == OK);
			scans[i] = hf[i]->openScan(s);
			assert(s == OK);
		}
		char* name;
		makeHFname(name,passNum,tempHFnum);
		tempHFnum++;
		HeapFile* outHF = new HeapFile(name,s);
		assert(s == OK);
		merge(scans,scan_num,outHF);
		if(SORTDEBUG)cout<<outHF->getRecCnt()<<endl;
		tempHFs.emplace(name);
		delete []name;
		for(int i=0;i<scan_num;++i){
			hf[i]->deleteFile();
			delete hf[i];
			hf[i] = nullptr;
		}
		delete outHF;
		delete [] hf;
	}
	if(SORTDEBUG)cout<<"pass "<<passNum<<" end"<<endl;
	return OK;
}

Status Sort::merge(Scan* scan[],int runNum,HeapFile* outHF){
	char* recodes = new char[runNum*rec_len];
	int* runFlag = new int[runNum]{0};
	RID rid;
	int runId;
	int scaned_num=0;
	int len;
	for(int i=0;i<runNum;++i){
		assert(scan[i]->getNext(rid,recodes+i*rec_len,len)==OK);
	}
	while(1){
		popup(recodes,runFlag,runNum,runId);
		assert(outHF->insertRecord(recodes+runId*rec_len,rec_len,rid)==OK);
		if(scan[runId]->getNext(rid,recodes+runId*rec_len,len)==DONE){
			runFlag[runId] = 1;
			scaned_num++;
		}
		if(scaned_num == runNum){
			break;
		}
	}
	delete[] recodes;
	delete[] runFlag;
	return OK;
}

Status Sort::popup(char* record,int *runFlag,int runNum,int& runId){
	for(int i=0;i<runNum;++i){
		if(runFlag[i]==0){
			runId = i;
			break;
		}
	}
	for(int i=runId+1;i<runNum;++i){
		if(runFlag[i]==1){
			continue;
		}
		if(tupleCmp(record+i*rec_len,record+runId*rec_len)==-1){
			runId = i;
		}
	}
}