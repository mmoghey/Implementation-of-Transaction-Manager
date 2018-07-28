/* main program that processes input and invokes the Tx mgr methods */
// the main data structures are initialized here

#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string>
#include <fstream>

#include "transx_def.hpp"
#include "transx_manager.hpp"
#include "transx_global.hpp"
#include "transx_extern.hpp"

#define MAX_TOKENS 6
#define MAX_INPUT_STRING 100

#include<stddef.h>
#define READWRITE 0

//Read the string from txt
 void Tokenize(const string& str,string tokens[MAX_TOKENS],
                      const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);
        int i =0;
    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the array.
        tokens[i] = str.substr(lastPos,pos-lastPos);
        i++;

        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        
	    // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}


// change the string number to int
int string2int(char *s,string str)
{
	int i =0;
 	s = (char *) malloc (sizeof(char) * 12);
 	while (str[i]!='\0')
  	{
    	s[i] = str[i];
    	i++;
  	}
  	int k = atoi(s);
   	return(k);
}




//main test function
int main(int argn, char **argv){
  	long sgno=1, obno,tid;
  	char lockmode,Txtype, temp[2]; //to convert str to char
  	char *infilename;
  	transx *t;
  	string delimiters = " ";
  	char str[MAX_INPUT_STRING];
  	string s;
  	int op;
  	char *c;
  	int thrNum =0;//count of # threads created

  	if (argn < 2) {
		printf("USAGE:\n");
		printf("\ttransx_test <input file name WITH extension>\n" ) ;
		exit(1);
	}

  	infilename = argv[1];
  	ifstream inFile(infilename,ios::in);

 
	if (! inFile.is_open()){ 
		cout << "\nError opening file: " << infilename << ".txt\n";
  		exit (1); 
	}

	//if invoked correctly, create one transaction manager object
	//also the hash table used as lock table

 	t_manager = new transx_manager();
 	hash_t1 = new transx_hash_table(DEFAULT_HASH_TABLE_SIZE);
 
    inFile.getline (str,MAX_INPUT_STRING);
    while (!inFile.eof() )  {   
        s = str;
        cout << s << "\n";
        string tokens[MAX_TOKENS];
        Tokenize(s, tokens);

       

      	if (tokens[0] == "//" ) 
			cout << s << "\n";
       	
		else if(tokens[0] =="Log" || tokens[0] == "log"){
	 		cout << "Log file name:" << tokens[1] << "\n\n";  
	 		//	printf("\nLog :", tokens[1]);
         	t_manager->openlog(tokens[1]);
        } 
		
		else if(tokens[0]=="BeginTx" || tokens[0] == "begintx"){
			tid = string2int(c,tokens[1]);
     		//Fall 2014[jay]. added following two statements to get Txtype
     		//Fall 2014[jay]. Passing the Txtype to get the transaction type.
       		strcpy(temp,tokens[2].c_str());
       		Txtype = temp[0];
	 
			printf("BeginTx : %d\n\n", tid);
	 		printf("TxType : %c\n\n", Txtype);
   			//cout << "THREAD ID  HERE: "<<thrNum << endl; 
	 		
			if ((op=t_manager->BeginTx(tid, thrNum++,Txtype))<0)
	  			cout << "\nerro from:" << tokens[0] <<" for TID:" << tid << "\n";
       	}
    	
		else if(tokens[0]=="Read" || tokens[0] == "read") {
	 		int k = string2int(c,tokens[1]);
			tid = k;
	 
			k = string2int(c,tokens[2]);
			obno = k;
			//    cout << "\nRead: " << tid << " : " << obno;
			printf("Read : %d : %d\n\n", tid, obno);
    		//cout << "tthread "<<thrNum << endl; 
			if((op= t_manager->TxRead(tid,obno, thrNum++))<0)
				cout << "\nerro from:" << tokens[0] <<" for TID:" << tid << "\n";
		}
		
		else if(tokens[0]=="Write" ||  tokens[0] == "write") {
 			int k = string2int(c,tokens[1]);
 			tid = k;
 
 			k = string2int(c,tokens[2]);
 			obno = k;
	 
 			//      cout << "\nwrite: " << tid << " : " << obno;
 			printf("Write : %d : %d\n\n", tid, obno);
 			if((op= t_manager->TxWrite(tid,obno, thrNum++))<0)
   				cout << "\nerro from:" << tokens[0] <<" for TID:" << tid << "\n";
  		}
  		
		else if(tokens[0]=="Abort" || tokens[0] == "abort") {
 			int k = string2int(c,tokens[1]);
 			tid = k;
 
 			//     cout << "\nAbort: " << tid;
 			printf("Abort : %d\n\n", tid);
			
			if((op= t_manager->AbortTx(tid, thrNum++))<0)
   				cout << "\nerro from:" << tokens[0] <<" for TID:" << tid << "\n";
   		}
  		
		else if(tokens[0]=="Commit" || tokens[0] == "commit") {
   			//cout << "test1234" << endl; 
   			int k = string2int(c,tokens[1]);
   			tid = k;
   			printf("Commit : %d\n\n", tid);
   			if((op= t_manager->CommitTx(tid, thrNum++))<0)
   				cout << "\nerro from:" << tokens[0] <<" for TID:" << tid << "\n";
     	}
       	
       	else if(tokens[0]=="end" || tokens[0] == "End") {
	    	printf("Release all resources and exit:\n\n");
            fflush(stdout);
            if((op= t_manager->endTm(thrNum++))<0)
	        	cout << "\nerro from:" << tokens[0] <<" for TID:" << tid << "\n";
               	fflush(stdout);
	   	    	inFile.close();
            	pthread_exit(NULL);
        
		} else {
	 		cout << "\ninput error:" << tokens[0] << "," << tokens[1] << "," << tokens[2] << "," << tokens[3] << "\n"; 
			cout << endl;  
   			fflush(stdout); 
			inFile.close();
 			pthread_exit(NULL);
  		}
  		
		inFile.getline (str,100);
   	}
   	
	cout << endl; 
   	inFile.close();
   	pthread_exit(NULL);
}






