#include <iostream>
#include <bits/stdc++.h>
using namespace std;

//mapping 8 bit registers to their values
map<string,unsigned char*> smallRegisters;
//mapping 16 bit registers to their values
map<string,unsigned short*> bigRegisters;
//places instruction lines in a vector
vector<string> lines;
//mapping all register, variables according to 8 or 16  bits
map<string,int> sizes;
//mapping label lines to their lines vector location
map<string,int> labels;
//vars; mapping variable names to their memory locations
map<string,int> memLoc;
//an array to hold char values
unsigned char memory[2<<15] ;    // 64K memory
//stores where instruction bytes ends
int memStartingPoint=0;
//holds inputs
string inputs;
//if current command is inc or dec
bool isIncorDec=false;
//stores valid commands
set<string> commands {"mov","add","sub","inc","dec","mul","div","xor","or","and","not","rcl","rcr","shl","shr","push","pop",
	"nop","cmp","jz","jnz","je","jne","ja","jae","jb","jbe","jnae","jnb","jnbe","jnc","jc","push","pop","int"};
//if we catch an error
bool isError=false;

unsigned short ax = 0 ;
unsigned short bx = 0 ;
unsigned short cx = 0 ;
unsigned short dx = 0 ;
unsigned short di = 0 ;
unsigned short si = 0 ;
unsigned short bp = 0 ;
unsigned short sp = (2<<15)-2 ;

bool zf= false;
bool sf= false;
int cf= 0;
bool af= false;
bool of= false;

//map ragisters to their values
void mapRegisters() {
    bigRegisters.insert(make_pair("ax",&ax));
    bigRegisters.insert(make_pair("bx",&bx));
    bigRegisters.insert(make_pair("cx",&cx));
    bigRegisters.insert(make_pair("dx",&dx));
    bigRegisters.insert(make_pair("di",&di));
    bigRegisters.insert(make_pair("si",&si));
    bigRegisters.insert(make_pair("bp",&bp));
    bigRegisters.insert(make_pair("sp",&sp));
    smallRegisters.insert(make_pair("ah",(unsigned char *) ( ( (unsigned char *) &ax) + 1)));
    smallRegisters.insert(make_pair("al",(unsigned char *) &ax));
    smallRegisters.insert(make_pair("bh",(unsigned char *) ( ( (unsigned char *) &bx) + 1)));
    smallRegisters.insert(make_pair("bl",(unsigned char *) &bx));
    smallRegisters.insert(make_pair("ch",(unsigned char *) ( ( (unsigned char *) &cx) + 1)));
    smallRegisters.insert(make_pair("cl",(unsigned char *) &cx));
    smallRegisters.insert(make_pair("dh",(unsigned char *) ( ( (unsigned char *) &dx) + 1)));
    smallRegisters.insert(make_pair("dl",(unsigned char *) &dx));
}
//converts decimal number which are in string formats to their binary values
string d2b(const string &s) {
    string bin;
    int dec=stoi(s);
    int size=0;
    while(dec!=0) {
        string temp;
        temp+=((char)(dec%2+48));
        bin.insert(0,temp);
        dec/=2;
        size++;
    }
    if(size<16) {
        for(int i=0; i<16-size; i++) {
            string temp="0";
            bin.insert(0,temp);
        }
    }
    return bin;
}
//splits a line into tokens and returns a vector filled with tokens
void tokenize(vector <string>& tokens,string  a) {
    //strange checks if we have comma or space as character, becausw if we do i have to seperate them from regular space and commas
    bool strange=false;
    int pos=0;
    //keeps space and commas which used as characters
    string spare;
    if(a.find("' '")<a.size() or a.find("','")<a.size()) {
        pos=a.find("' '")+a.find("','")+1;
        strange=true;
        for(int i=pos; i<a.size(); i++) {
            spare+=a[i];
        }
        a.erase(pos);
    }
    string s;
    bool flag=false;
    for(char i : a) {
        if(i==' ' or i==',') {
            if(!s.empty()) {
                if((s[s.size()-1]=='[' or s[0]=='[') and s[s.size()-1]!=']') {
                    flag=true;
                    tokens.push_back(s);
                } else if (s[s.size()-1]==']' and flag) {
                    tokens[tokens.size()-1]+=s;
                    flag=false;
                } else {
                    if(flag) {
                        tokens[tokens.size()-1]+=s;
                    } else {
                        tokens.push_back(s);
                    }
                }
            }
            s="";
        } else {
            s+=i;
        }
    }
    if((!s.empty() and s[s.size()-1]==']')and flag) {
        tokens[tokens.size()-1]+=s;
    } else if(!s.empty()) {
        tokens.push_back(s);
    }
    if(strange) {
        tokens.push_back(spare);
    }

}
//combines some tokens such as "b" and "[1235]" which are meant to be together
void compress(vector<string> &tokens , const int & start, int &size) {
    for(int j=0; j<size; j++) {
        if((tokens[j]=="w" or tokens[j]=="b") and j!=size-1) {
            tokens[j]+=" "; tokens[j]+=tokens[j+1];
            for(int k=j+1; k<size-1; k++) {
                tokens[k]=tokens[k+1];
            }
            size--;
            compress(tokens,j+1,size);
        } else if(tokens[j]=="offset" and j!=size-1) {
            tokens[j]+=" "; tokens[j]+=tokens[j+1];
            for(int k=j+1; k<size-1; k++) {
                tokens[k]=tokens[k+1];
            }
            size--;
            compress(tokens,j+1,size);
        } else if(tokens.size()>1 and tokens[j][1]=='[') {
            tokens[j]+=" ";
            for(int i=tokens[j].size()-2; i>0 ; i--) {
                tokens[j][i+1]=tokens[j][i];
            }
            tokens[j][1]=' ';
        }
    }
}
//checks if two lines are equal
bool isEqual(string s1,string s2) {
    vector<string> tokens1, tokens2;
    tokenize(tokens1,s1); tokenize(tokens2,s2);
    if(tokens1.size()!=tokens2.size())
        return false;
    for(int i=0; i<tokens1.size(); i++) {
        if(tokens1[i]!=tokens2[i])
            return false;
    }
    return true;
}
//converts hexadecimal number which are in string formats to their decimal values
int h2d(const string& hex) {
    map<char,int> hexval;
    char a='0'; int k=0;
    for(int i=0; i<16; i++) {
        hexval.insert(make_pair(a,k));
        a++; k++;
        if(k==10)
            a='a';
    }
    int coeff=1;
    int size=hex.size();
    int dec=0;
    for(int i=size-2; i>=0; i--) {
        dec+=hexval[hex[i]]*coeff;
        coeff*=16;
    }
    return dec;
}
//converts binary number which are in string formats to their decimal values
int b2d(const string& bin) {
    int coeff=1;
    int size=bin.size();
    int dec=0;
    for(int i=size-1; i>=0; i--) {
        dec+=((int)bin[i]-48)*coeff;
        coeff*=2;
    }
    return dec;
}
//converts a string which we know holds a number in various ways to their decimal values
int s2d(const string& number) {
    int result=0;
    if(number[number.size()-1]=='h' or number[0]=='0' or number[0]=='a' or number[0]=='b' or number[0]=='c' or number[0]=='d' or number[0]=='e' or number[0]=='f') {
        if(number[number.size()-1]!='h') {
            string temp=number+"h";
            result=h2d(temp);
        } else {
            result=h2d(number);
        }
    } else if(number[number.size()-1]=='b') {
        result=b2d(number);
    } else if(number[0]==39) {
        result=number[1];
    } else if(number[number.size()-1]=='d') {
        string temp=number;
        temp.pop_back();
        result=stoi(temp);
    } else {
        result=stoi(number);
    }
    return result;
}
//places variables to memory and maps them about their sizes according to tokens vector which holds tokens from variable defining line
void placeVariables(vector<string> &tokens, int &a) {
    string name=tokens[0];
    string type=tokens[1];
    string value=tokens[2];
    memLoc.insert(make_pair(name,a));
    //dval for decimal value
    int dval=s2d(value);
    if(type[1]=='b' and dval<256) {
        sizes.insert(make_pair(name,8));
        memory[a]=dval;
        a++;
    } else if(type[1]=='w' && dval<65536) {
        sizes.insert(make_pair(name,16));
        auto first=(unsigned char) dval , second=(unsigned char)(dval/256);
        memory[a]=first ; memory[a+1]=second;
        a+=2;
    }
}
//mapping registers according to 8 or 16 bits
void placeRegisters() {
    string a="ax";
    string b="ah", c="al";
    for(int i=0; i<4; i++) {
        sizes.insert(make_pair(a,16));
        a[0]++;
    }
    sizes.insert(make_pair("di",16));
    sizes.insert(make_pair("si",16));
    sizes.insert(make_pair("bp",16));
    sizes.insert(make_pair("sp",16));
    for(int i=0; i<4; i++) {
        sizes.insert(make_pair(b,8));
        sizes.insert(make_pair(c,8));
        b[0]++, c[0]++;
    }

}
//removes space characters from the beginning of a line
void removeStartingWhiteSpaces(string &l) {
    int size=l.size();
    for(int i=0; i<size; i++) {
        if(l[i]!=' ') {
            break;
        }
        l.erase(0,1);
        i--;
    }
}
//returns third bit of a number
int thirdBit(int num) {
    return (num/8)%2;
}
//returns last bit of a number
int lastBit(int num , int size) {
    return num/(int)pow(2,size-1);
}
//sets flags after an operation
void setFlags(const string& opType, int size, int src, int dest, int result) {
    int max=pow(2,size);
    //t means two's complement
    int tDest=dest-max*(dest/(max/2)) , tSrc=src-max*(src/(max/2));
    int tResult=0;
    result==0 ? zf=true : zf=false;
    lastBit(result,size)>0 ? sf=true : sf = false;
    if(opType=="add") {
        tResult=tDest+tSrc;
        if(!isIncorDec) {
            result<dest ? cf=1 : cf=0;
        }
        thirdBit(dest)+thirdBit(src)>thirdBit(result) ? af=true : af=false;
        tResult > ((max/2) -1) or tResult< -1*(max/2) ?of=true : of =false;
    } else if(opType=="sub" or opType=="cmp") {
        tResult=tDest-tSrc;
        if(!isIncorDec) {
            dest<src ? cf=1 : cf=0;
        }
        thirdBit(dest)-thirdBit(src)<thirdBit(result) ? af=true : af=false;
        tResult > ((max/2) -1) or tResult< -1*(max/2) ?of=true : of =false;
    } else if(opType=="and" or opType=="or" or opType=="xor") {
        cf=0, of=false;
    }
}
//decides what a string represents
string decideToWhat(string& name) {
    if(bigRegisters.count(name)!=0) {
        return "br"; // br means bigRegister
    } else if(smallRegisters.count(name)!=0) {
        return "sr"; // sr means smallRegister
    } else if (sizes.count(name)!=0) {
        if(sizes[name]==8) {
            return "sv"; // sv means small variable
        } else {
            return "bv"; // bv means 16 bit variable
        }
    } else if(name[name.size()-1]==']' and ((name[0]=='[') or (name[2]=='[') )) {
        name.erase(name.size()-1);
        if(name[0]=='[') {
            name.erase(0,1);
            return "ma"; //ma means memory address
        } else if(name[0]=='b') {
            name.erase(0,3);
            return "msa"; //msa means memory single address
        } else {
            name.erase(0,3);
            return "mda"; //mda means memory double address
        }
    } else if(name.find("offset")<name.size()) {
        int x=name.find_first_of('t');
        int start=0;
        for(int i=x+1 ; i<name.size(); i++) {
            if(name[i]!=' ') {
                start=i;
                break;
            }
        }
        name.erase(0,start);
        return "ml"; // ml means memory location
    }else if((name[0]=='b' or name[0]=='w') and name[1]==' ') {
        char c=name[0]; name.erase(0,2);
        if(c=='b') return "sv";
        return "bv";
    } else {
        return "imm"; // imm means immediate value
    }
}
//returns value of a something according to parameters: size of source,some initial value, type of source and name of source
int getSrcValue(int size, int src , string & srcType, string & srcName) {
    if(((size==8 or size==0) and srcType=="sr")  or ((size==16 or size==0) and srcType=="br")) {
        srcType=="sr" ? src=*smallRegisters[srcName] : src=*bigRegisters[srcName];
    } else if(((size==8 or size==0) and srcType=="sv")  or ((size==16 or size==0) and srcType=="bv")) {
        int first=memory[memLoc[srcName]]; int second=memory[memLoc[srcName]+1];
        srcType=="sv" ? src=first : src=second*256+first;
    } else if(srcType=="ma" or srcType=="msa" or srcType=="mda") {
        int loc=0;
        if(bigRegisters.count(srcName)!=0) { // means [reg]
            loc=*bigRegisters[srcName];
        } else {
            loc=s2d(srcName); // means [memory]
        }
        if(loc<memStartingPoint or loc>65535) {
            isError=true;
            cout<<"Error ";
            return 0;
        }
        int first=memory[loc]; int second=memory[loc+1];
        if(srcType=="ma") {
            (size==8 or size==0) ? src=first : src=second*256+first;
        } else if(srcType=="msa" and size==8) {
            src=first;
        } else if(srcType=="mda" and size==16) {
            src=second*256+first;
        } else {
            isError=true;
            cout<<"Error ";
            return 0;
        }
    } else if(srcType=="ml") {
        src=memLoc[srcName];
    } else if((size==8 and (srcType=="br" or srcType=="bv" or srcType=="mda")) or (size==16 and (srcType=="sr" or srcType=="sv" or srcType=="msa"))) {
        isError=true;
        cout<<"Error ";
        return src;
    } else {
        src=s2d(srcName); //can give an Error
    }
    return src;
}
//takes to string which are in binary format and returns "and" operation of them
string andd(string s1,string s2) {
    string s;
    for(int i=0; i<s1.size(); i++) {
        (s1[i]-48)*(s2[i]-48)==0 ? s+="0" : s+="1";
    }
    return s;
}
//takes to string which are in binary format and returns "or" operation of them
string orr(string s1,string s2) {
    string s;
    for(int i=0; i<s1.size(); i++) {
        (s1[i]-48)+(s2[i]-48)==0 ? s+="0" : s+="1";
    }
    return s;
}
//takes to string which are in binary format and returns "xor" operation of them
string xorr(string s1,string s2) {
    string s;
    for(int i=0; i<s1.size(); i++) {
        ((s1[i]-48)+(s2[i]-48))%2==0 ? s+="0" : s+="1";
    }
    return s;
}
//takes destination and source names and their sizes and gives the outcome of operations which can be "and","or","xor"
int and_or_xor(const string& opType,int size, int dest, int source) {
    string d,s;
    if(size==8) {
        dest%=256;
        source%=256;
    } else {
        dest%=65536;
        source%=65536;
    }
    d=d2b(to_string(dest));
    s=d2b(to_string(source));

    if(opType=="and") return b2d(andd(d,s));
    if(opType=="or") return b2d(orr(d,s));
    return b2d(xorr(d,s));
}
//takes destination and size of it and gives the outcome of operations which can be "not","shl","shr","rcl","rcr"
int notFalan(string opType, int dest, int size) {
    int result=0;
    int coeff=1;
    if(opType=="not") {
        for(int i=0; i<size; i++) {
            result+=coeff*(1-(dest%2));
            dest/=2;
            coeff*=2;
        }
        return result;
    } else if(opType=="shr") {
        cf=dest%2;
        return dest/2;
    } else if(opType=="shl") {
        cf=lastBit(dest,size);
        result=dest*2;
        if(size==8) return (unsigned char)result;
        return (unsigned short) result;
    } else if(opType=="rcr") {
        int temp=cf;
        cf=dest%2;
        return (int)(temp*pow(2,size-1))+(int)(dest/2);
    } else if(opType=="rcl") {
        int temp=cf;
        cf=lastBit(dest,size);
        result=dest*2+temp;
        if(size==8) result%=256;
        if(size==16) result%=65536;
        return result;
    }
    return result;
}
//operates not,rcl,rcr,shr,shl operations accoring to parameters
void not_rcl_rcr_shr_shl(const string& opType, string reg,int times) { // times for how many times, like "rcl,5"
    string type=decideToWhat(reg);
    int value=0;
    if(type=="br" or type=="bv" or type=="mda") {
        value=getSrcValue(16,value,type,reg);
        for(int i=0; i<times; i++) {
            value=notFalan(opType,value,16);
        }
        if(times==1 and opType=="rcl") {
            (cf+lastBit(value, 16))%2==1 ? of= true : of=false;
        } else if(times==1 and opType=="rcr") {
            (lastBit(value, 16) + (value/16384)%2)%2==1 ? of= true : of=false;
        } else if(times==1 and opType=="shl") {
            lastBit(value,16)==cf ? of= false : of=true;
        } else if(times==1 and opType=="shr") {
            (value/16384)%2==1 ? of=true : of=false;
        }
        if(type=="br") {
            *bigRegisters[reg]=value;
        } else if(type=="bv") {
            memory[memLoc[reg]]=value%256;
            memory[memLoc[reg]+1]=value/256;
        } else {
            memory[stoi(reg)]=value%256;
            memory[stoi(reg)+1]=value/256;
            }
    } else if(type=="sr" or type=="sv" or type=="msa" or type=="ma") {
        value=getSrcValue(8,value,type,reg);
        for(int i=0; i<times; i++) {
            value=notFalan(opType,value,8);
        }
        if(times==1 and opType=="rcl") {
            (cf+lastBit(value, 8))%2==1 ? of= true : of=false;
        } else if(times==1 and opType=="rcr") {
            (lastBit(value, 8) + (value/64)%2)%2==1 ? of= true : of=false;
        } else if(times==1 and opType=="shl") {
            lastBit(value,8)==cf ? of= false : of=true;
        } else if(times==1 and opType=="shr") {
            (value/64)%2==1 ? of=true : of=false;
        }
        if(type=="sr") {
            *smallRegisters[reg]=value;
        } else if(type=="sv") {
            memory[memLoc[reg]]=(unsigned char)value;
        } else {
            memory[stoi(reg)]=(unsigned char)value;
        }
    } else {
        isError=true;
        cout<<"Error ";
        return;
    }
}
//means any operation in which destination is a register (smth2reg stands for something to register)
void smth2reg(const string& opType,const string& destination,const string& source) {
    string temp=source;
    string srcType=decideToWhat(temp);
    int dest =0 , src=0, result =0 ,size=sizes[destination];
    size==8 ? dest=*smallRegisters[destination] : dest=*bigRegisters[destination];
    //time to decide what's hidden in the source
    src=getSrcValue(size,src,srcType,temp);
    if(opType=="add") {
        size==8 ? *smallRegisters[destination]+=src : *bigRegisters[destination]+=src;
    }else if (opType=="sub" or opType=="cmp"){
        size==8 ? *smallRegisters[destination]-=src : *bigRegisters[destination]-=src;
    } else if(opType=="and" or opType=="or" or opType=="xor") {
        size==8 ? *smallRegisters[destination]=and_or_xor(opType,8,*smallRegisters[destination],src) : *bigRegisters[destination]=and_or_xor(opType,16,*bigRegisters[destination],src);
    } else {
        if(srcType=="imm" and src>pow(2,size)-1) {
            isError=true;
            cout<<"Error"; return;
        }
        size==8 ? *smallRegisters[destination]=src : *bigRegisters[destination]=src;
    }
    size==8 ? result=*smallRegisters[destination] : result=*bigRegisters[destination];
    if(opType=="cmp") {
        size==8 ? *smallRegisters[destination]=dest : *bigRegisters[destination]=dest;
    }
    setFlags(opType,size,src,dest,result);
}
//means any operation in which destination is a memory location or a variable because variables are also stored in memory (smth2mem stands for something to memory)
void smth2mem(const string& opType, const string& destination, const string& source) {
    string temp=source, temp2=destination;
    //temp2 holds the string that hidden in [] and destType is b[] or w[]
    string srcType=decideToWhat(temp) , destType=decideToWhat(temp2);
    int dest =0 , src=0, result =0 ,size=0, loc;
    if(bigRegisters.count(temp2)!=0) { //means temp2 is register, which means b/w[reg]
        loc=*bigRegisters[temp2];
    } else { // means temp2 is only a number , which means b/w[memory]
        loc=s2d(temp2);
    }
    if(loc>65535) {
        isError=true;
        cout<<"Error"; return;
    }
    if(destType=="msa") {
        size=8;
    } else if(destType=="mda") {
        size=16;
    } else {
        isError=true;
        cout<<"Error ";
        return;
    }
    src=getSrcValue(size,src,srcType,temp);
    unsigned char first= src%256; unsigned char second=(src/256);
    if(opType=="add") {
        if(size==8) {
            memory[loc]+=first;
            result=memory[loc];
        } else {
            unsigned short atFirst=memory[loc] + memory[loc+1]*256;
            atFirst+=src;
            first= atFirst; second=(atFirst/256);
            memory[loc]=first; memory[loc+1]=second;
            result=atFirst;
        }
    } else if(opType=="sub" or opType=="cmp") {
        if(size==8) {
            memory[loc]-=first;
            result=memory[loc];
        } else {
            unsigned short atFirst=memory[loc] + memory[loc+1]*256;
            atFirst-=src;
            first= atFirst; second=(atFirst/256);
            memory[loc]=first; memory[loc+1]=second;
            result=atFirst;
        }
    } else if(opType=="and" or opType=="or" or opType=="xor") {
        if(size==8) {
            memory[loc]=and_or_xor(opType,size,memory[loc],first);
            result=memory[loc];
        } else {
            unsigned short atFirst=memory[loc] + memory[loc+1]*256;
            atFirst=and_or_xor(opType,size,atFirst,src);
            first= atFirst; second=(atFirst/256);
            memory[loc]=first; memory[loc+1]=second;
            result=atFirst;
        }
    } else { //mov
        if(size==8) {
            memory[loc]=first;
            result=memory[loc];
        } else {
            memory[loc]=first; memory[loc+1]=second;
            result=(unsigned short)src;
        }
    }
    if(opType=="cmp") {
        if(size==8) {
            memory[loc]+=first;
        } else {
            unsigned short atFirst=memory[loc] + memory[loc+1]*256;
            atFirst+=src;
            first= atFirst%256; second=(atFirst/256);
            memory[loc]=first; memory[loc+1]=second;
        }
    }
    setFlags(opType,size,src,dest,result);
}
//performs  one of those operations
void mov_add_sub_and_or_xor_cmp(const string& opType ,const string& reg1,const string& reg2) {
    string tempReg1=reg1; string typeOfDest=decideToWhat(tempReg1);
    if(typeOfDest=="br" or typeOfDest=="sr") {
        smth2reg(opType,reg1,reg2);
    } else if(typeOfDest=="bv" or typeOfDest=="sv") {
        string temp="b ["+to_string(memLoc[reg1])+"d]";
        typeOfDest=="bv" ? temp[0]='w' : temp[0]='b' ;
        smth2mem(opType,temp,reg2);
    } else if(typeOfDest=="msa" or typeOfDest=="mda" or typeOfDest=="ma") {
        smth2mem(opType,reg1,reg2);
    }
}
//performs multiplication and divison
void mul_div(const string& opType, string reg) {
    string type=decideToWhat(reg);
    int size=0 , value=0;
    if(type=="br" or type=="bv" or type=="mda") {
        size=16; value=getSrcValue(size,value,type,reg);
    } else if(type=="sr" or type=="sv" or type=="msa") {
        size=8; value=getSrcValue(size,value,type,reg);
    } else {
        isError= true;
        cout<<"Error ";
        return;
    }
    if(opType=="mul") {
        if(size==8) {
            *bigRegisters["ax"]=*smallRegisters["al"]*value;
            *smallRegisters["ah"]==0 ? cf=0 , of=false : cf=1 , of=true;
        } else {
            int result=*bigRegisters["ax"]*value;
            *bigRegisters["ax"]=result%65536;
            *bigRegisters["dx"]=result/65536;
            *bigRegisters["dx"]==0 ? cf=0 , of=false : cf=1 , of=true;
        }
    } else {
        int dividend =0 , quotient=0 ,remainder=0;
        if(size==8) {
            dividend=*bigRegisters["ax"];
            quotient=dividend/value; remainder=dividend%value;
            if(quotient>255) {
                isError=true;
                cout<<"Error "; of=true; cf=1; return;
            }
            *smallRegisters["al"]=quotient;
            *smallRegisters["ah"]=remainder;
        } else {
            dividend=(*bigRegisters["dx"]*65536)+(*bigRegisters["ax"]);
            quotient=dividend/value; remainder=dividend%value;
            if(quotient>65535) {
                isError= true;
                cout<<"Error "; of=true; cf=1; return;
            }
            *bigRegisters["ax"]=quotient;
            *bigRegisters["dx"]=remainder;
        }
    }
}
//performs push and pop
void push_pop(const string& opType, string reg) {
    string type=decideToWhat(reg);
    int value=0;
    if(opType=="push") {
        if(type=="br" or type=="mda" or type=="imm") {
            value=getSrcValue(16,value,type,reg);
            memory[sp]=value%256;
            memory[sp+1]=value/256;
            sp-=2;
        } else {
            isError=true;
            cout<<"Error ";
            return;
        }
    } else {
        value=memory[sp+3]*256+memory[sp+2];
        if(type=="br") {
            *bigRegisters[reg]=(unsigned short)value;
        } else if(type=="mda") {
            int loc=s2d(reg);
            memory[loc]=(unsigned char)value%256;
            memory[loc+1]=(unsigned char)value/256;
        } else {
            isError=true;
            cout<<"Error ";
            return;
        }
        sp+=2;
    }
}
//turns all uppercase characters into lowercase in a line
void toLowerChars(string & line) {
    //isBetween is used for to detect a character is between '', because i can't modify them
    bool isBetween=false;
    for(char & i : line) {
        i==39 ? isBetween=!isBetween : isBetween=isBetween;
        if((i>64 and i<91) and !isBetween) {
            i+=32;
        }
    }
}

//reads file and places everyting like maps,vector,memory array...
void readFile(ifstream& inFile) {
    placeRegisters();
    mapRegisters();
    string l;
    int a=0;
    int b=0;
    while(getline(inFile,l)) {
        removeStartingWhiteSpaces(l);
        toLowerChars(l);
        if(isEqual(l,"code segment")) {
            break;
        }
    }
    while(getline(inFile,l)) {
        removeStartingWhiteSpaces(l);
        toLowerChars(l);
        if(isEqual(l,"int 20h")) {
            break;
        }
        lines.push_back(l);
        if(l[l.size()-1]==':') {
            l.pop_back();
            labels.insert(make_pair(l,a));
            b++;
        }
        a++;
    }
    a=(a-b)*6;
    memStartingPoint=a;
    while(getline(inFile,l)) {
        toLowerChars(l);
        if(isEqual(l,"code ends")) {
            break;
        }
        vector<string> tokens;
        tokenize(tokens,l);
        placeVariables(tokens, a);
    }
}
//executes code
void executeCode(int start) {
    for(int i=start; i<lines.size(); i++) {
        if (isError) {
            return;
        }
        string line=lines[i];
        if(line[line.size()-1]==':') {
            continue;
        }
        vector<string> tokens;
        tokenize(tokens,line);
        int size=tokens.size();
        compress(tokens,0,size);
        tokens.resize(size);
        if(tokens.size()>1 and commands.count(tokens[0])==0) {
            cout<<"Error"; return;
        }
        if(tokens.size()==3 and (tokens[0]=="mov" or tokens[0]=="add" or tokens[0]=="sub" or tokens[0]=="and" or tokens[0]=="or" or tokens[0]=="xor" or tokens[0]=="cmp" )) {
            mov_add_sub_and_or_xor_cmp(tokens[0],tokens[1],tokens[2]);
        } else if(tokens.size()==2 and (tokens[0]=="inc" or tokens[0]=="dec")) {
            isIncorDec=true;
            string x;
            tokens[0]=="inc" ? x="add"  : x="sub";
            mov_add_sub_and_or_xor_cmp(x,tokens[1],"1");
            isIncorDec=false;
        } else if((tokens.size()==2 and tokens[0]=="not") or ((tokens.size()==2 or tokens.size()==3) and (tokens[0]=="shl" or tokens[0]=="shr" or tokens[0]=="rcl" or tokens[0]=="rcr"))) {
            int times=1;
            if(tokens.size()==3) {
                string type=decideToWhat(tokens[2]);
                times=getSrcValue(0,times,type,tokens[2]);
            }
            not_rcl_rcr_shr_shl(tokens[0],tokens[1],times);
        } else if((tokens.size()==2) and (tokens[0]=="mul" or tokens[0]=="div")) {
            mul_div(tokens[0],tokens[1]);
        } else if((tokens.size()==2) and (tokens[0]=="push" or tokens[0]=="pop")) {
            push_pop(tokens[0],tokens[1]);
        }else if(tokens.size()==2 and labels.count(tokens[1])!=0) {
            string label=tokens[1]; int lineNo=labels[label];
            if(tokens[0]=="jmp") {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="jz" or tokens[0]=="je") and zf) {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="jnz" or tokens[0]=="jne") and !zf) {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="ja" or tokens[0]=="jnbe") and (cf==0 and !zf)) {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="jb" or tokens[0]=="jnae") and cf==1) {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="jae" or tokens[0]=="jnb" or tokens[0]=="jnc") and cf==0) {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="jae" or tokens[0]=="jnb" or tokens[0]=="jnc") and cf==0) {
                executeCode(lineNo+1);
                return;
            } else if((tokens[0]=="jbe" or tokens[0]=="jna") and (cf==1 or zf)) {
                executeCode(lineNo+1);
                return;
            } else if(tokens[0]=="jc" and cf==1) {
                executeCode(lineNo+1);
                return;
            }
            else {
                continue;
            }
        } else if(tokens.size()==1 and tokens[0]=="nop") {
            continue;
        } else if(isEqual(line,"int 21h")) {
            if(*smallRegisters["ah"]==1) {
                getline(cin,inputs);
                *smallRegisters["al"]=inputs[0];
            } else if(*smallRegisters["ah"]==2) {
                cout<<*smallRegisters["dl"];
                *smallRegisters["al"]=*smallRegisters["dl"];
            }
        }
    }
}

int main(int argc, char* argv[]){
    ifstream inFile(argv[1]);
    readFile(inFile);
    executeCode(0);
}