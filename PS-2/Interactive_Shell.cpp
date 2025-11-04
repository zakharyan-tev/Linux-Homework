#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;

vector<string> split(const string& s, char d) {
    vector<string> r; string part; stringstream ss(s);
    while(getline(ss, part, d)) if(!part.empty()) r.push_back(part);
    return r;
}

vector<string> split2(const string& s, const string& t) {
    vector<string> r;
    size_t p = s.find(t);
    if(p==string::npos) r.push_back(s);
    else {
        r.push_back(s.substr(0,p));
        r.push_back(s.substr(p+t.size()));
    }
    return r;
}

int run(string cmd, int fd=1) {
    pid_t pid=fork();
    if(pid==0){
        if(fd!=1){dup2(fd,1);close(fd);}
        auto args=split(cmd,' ');
        vector<char*> a;
        for(auto& x:args) a.push_back((char*)x.c_str());
        a.push_back(nullptr);
        execvp(a[0],a.data());
        cerr<<"exec err\n";
        exit(1);
    }else if(pid>0){
        int st; waitpid(pid,&st,0);
        if(WIFEXITED(st)) return WEXITSTATUS(st);
    }else{
        cerr<<"fork err\n";
        exit(1);
    }
    return 0;
}

int handle(string cmd){
    if(cmd.find("&&")!=string::npos){
        auto p=split2(cmd," && ");
        if(handle(p[0])==0) handle(p[1]);
    }else if(cmd.find("||")!=string::npos){
        auto p=split2(cmd," || ");
        if(handle(p[0])!=0) handle(p[1]);
    }else if(cmd.find(';')!=string::npos){
        auto p=split2(cmd,"; ");
        handle(p[0]); handle(p[1]);
    }else if(cmd.find(">>")!=string::npos){
        auto p=split2(cmd," >> ");
        int f=open(p[1].c_str(),O_WRONLY|O_CREAT|O_APPEND,0644);
        run(p[0],f); close(f);
    }else if(cmd.find('>')!=string::npos){
        auto p=split2(cmd," > ");
        int f=open(p[1].c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);
        run(p[0],f); close(f);
    }else{
        return run(cmd);
    }
    return 0;
}

int main(){
    string in;
    while(true){
        cout<<"> ";
        if(!getline(cin,in) || in=="exit") break;
        if(!in.empty()) handle(in);
    }
    return 0;
}
