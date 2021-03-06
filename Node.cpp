#include "Node.h"
#include <fstream>

string ips[10] = {"127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1",
				 "127.0.0.1"};

string ports[10] = {"12341",
				 "12342",
				 "12343",
				 "12344",
				 "12345",
				 "12346",
				 "12347",
				 "12348",
				 "12349",
				 "12350"};

pair<string,string> split_(string NodeID) {
    string a="";int i;
    for(i=0;NodeID[i]!='<';i++) a+=NodeID[i];
    return pair<string,string>(a,NodeID.substr(i+1));
}

Node::Node(string ip, string port):ip(ip),port(port)
{ 
    ID=ip+string("<")+port; 
    Qmutex = new mutex();
    Qnotempty = new condition_variable();
    opFileMutex = new mutex();
    opFileCond = new condition_variable();
}

void Node::startUp(){
	thread listenMessage (&Node::receiveMessage,this);
    listenMessage.detach();

    thread listenFile(&Node::receiveFile,this);
    listenFile.detach();

    thread executeJob1(&Node::executeJob, this);
    executeJob1.detach();

    // thread submitJobT(&Node::submitJobThread, this);
    // submitJobT.detach();
    	
	for(int i=0; i<10; i++){
		if(port != ports[i]){
			string res = sendMessage(ips[i],ports[i],to_string(IAmUp)+"::"+ip+"<"+port);
			if(res == "error")
				continue;
			else if(res[0] == '2'){
                int idx = res.find("::");
				cout << "Peer alive at: " << res.substr(idx+2) << endl;
			}
		}
	}
	cout << "Total number of nodes in checkNodes : " <<  checkNodes.size() << endl;

    thread heartBeatMessage (&Node::checkAlive,this);
    heartBeatMessage.detach();
	
}

void Node::executeJob(){
    while(1){
        //cout<<getpid()<< " GQ : "<<globalQ.size()<<endl;
        unique_lock<mutex> lk{*Qmutex};
        while(globalQ.empty()) {
            Qnotempty->wait(lk);
        }
        Job job = globalQ.front();
        globalQ.pop_front();
        lk.unlock();
        cout << "*** Job is being executed ***" << endl;
        cout << "execFile name is: " << job.execFile << endl;
        cout << "ipFile name is: " << job.ipFile << endl;
        cout << "jobId is: " << job.jobId << endl;
        cout << "ownerId is: " << job.ownerId << endl;
        char* arglist[4];
        char arg1[MAX], arg2[MAX], arg3[MAX];
        strcpy(arg1,job.execFile.c_str());
        strcpy(arg2,job.ipFile.c_str());
        strcpy(arg3,(string("out_")+job.ipFile).c_str());
        arglist[0]=arg1;
        arglist[1]=arg2;
        arglist[2]=arg3;
        arglist[3] = NULL;
        char buf[MAX];
        if(job.execFile[0]=='/') 
            sprintf(buf,"%s",job.execFile.c_str());
        else 
            sprintf(buf,"%s/%s",get_current_dir_name(),job.execFile.c_str());
        
        int pid, status;
        if((pid = fork()) == 0){
            execvp(buf,arglist);
            cout << "execvp fail: " << errno << endl;
            _exit(-1);
        }
        waitpid(pid,&status,0);
        if(status!=0) {
            cout << "Execvp error: returned " << status << endl;
        }
        cout << "*** Job is finished ***" << endl;
        if(job.ownerId==ID) {
            mergeResult(ID,job.jobId,string("out_")+job.ipFile);
        } else {
            pair<string, string> addr = split_(job.ownerId);
            //cout << job.ipFile.size() << endl;
            //cout << addr.first << " " << addr.second << " " << string("out_")+job.ipFile<< endl;
            sendFile(addr.first, addr.second, string("out_")+job.ipFile);
            cout << "OK" << endl;
            sendMessage(addr.first, addr.second, to_string(Result)+"::"+ID+":"+job.jobId+":"+string("out_")+job.ipFile+":");
        }
    }
}

bool cmp(const pair<string,int>& p1,const pair<string,int>& p2) {
    return p1.second<p2.second;
}

void getDestNodes(vector<pair <string,int> >& load) {
    if(!load.size()) return;
    sort(load.begin(),load.end(),cmp);
    int mx=0;
    for(int i=0;i<load.size()-1;i++) mx=max(mx,load[i+1].second-load[i].second);
    if(!mx) return;
    for(int i=0;i<load.size()-1;i++) {
        if(load[i+1].second-load[i].second==mx) {
            load=vector<pair<string,int> >(load.begin(),load.begin()+i+1);
            return;
        }
    }
}

void Node::submitJob(string execFileName, string ipFileName,string newjid){
    //cout << execFileName << " " << ipFileName << endl;
    MD5 md5;
    char buf[MAX];
    unsigned long x = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    Job j;
    j.execFile=execFileName;
    j.ipFile=ipFileName;
    if(newjid.empty()) {
        strcpy(buf,execFileName.c_str());
        strcat(buf, to_string(x).c_str());
        string exf(md5.digestString(buf));
        strcpy(buf,ipFileName.c_str());
        strcat(buf, to_string(x).c_str());
        string ipf(md5.digestString(buf));
        j.jobId=exf+string("<")+ipf;
    } else {
        j.jobId = newjid;
    }
    cout << "Job ID: " << j.jobId << endl;
    j.ownerId=this->ID;
    md5_original[j.jobId]=execFileName+string("<")+ipFileName;
    //localQ.push_back(j);
    load.erase(load.begin(),load.end());
    //send load query to all nodes, receive reply, fill load
    for(int i=0; i<MACHINES; i++){
        if(ID == ips[i]+"<"+ports[i])
            continue;
        string out = sendMessage(ips[i],ports[i],to_string(Query)+"::"+ip+"<"+port);
        if(out == "timeout" or out == "disconnect")
            continue;
        int temp = stoi(out);
        load.push_back(make_pair(ips[i]+"<"+ports[i],temp));
    }
    cout << "Number of active peers: " << load.size() << endl;
    getDestNodes(load);
    Application app;
    vector<Job> vj= app.split(j,load.size()+1);
    cout << "Number of jobs spawned: " << vj.size() << endl;
    for(int i=0;i<vj.size()-1;i++) //send files to nodes in load[i]
    {
        string sentip = load[i].first.substr(0,load[i].first.find("<"));
        string sentport = load[i].first.substr(load[i].first.find("<")+1);
        sendJobMapping(sentip,sentport,vj[i].execFile,vj[i].ipFile,vj[i].jobId,vj[i].ownerId);
        // cout << "execFile " << vj[i].execFile << endl;
        // cout << "ipFile " << vj[i].ipFile << endl;
        sendFile(sentip, sentport, vj[i].ipFile);
        sendFile(sentip, sentport, j.execFile, vj[i].execFile);
        checkNodes.insert(load[i].first);
        vj[i].execFile = j.execFile;
        nodeToJobMap[load[i].first].push_back(vj[i]);
        jobToNodeMap[j.jobId].insert(load[i].first);
    }
    unique_lock<mutex> lk{*Qmutex};
    //keep self part
    if(globalQ.empty()) {
        globalQ.push_back(vj[vj.size()-1]);
        lk.unlock();
        Qnotempty->notify_all();
    } else {
        globalQ.push_back(vj[vj.size()-1]);
        lk.unlock();
    }
    cout << "Size of globalQ is " << globalQ.size() << endl;
    jobToNodeMap[j.jobId].insert(ID);
	
    cout << "Submit job done. File names are: " << execFileName << " " << ipFileName << endl;
}

void Node::checkAlive(){
    while(1){
        sleep(checkAliveTime);
        if(!checkNodes.empty())
            cout << "Sending heartbeat... \n";

        set<string>::iterator it;
        for(it = checkNodes.begin(); it != checkNodes.end();){
            string curNode = *it;
            pair<string,string> p=split_(curNode);
            cout << p.first << " " << p.second << endl;
            string res = sendMessage(p.first,p.second,to_string(CheckAlive)+"::"+curNode);
            if(res == "timeout" || res == "disconnect") {
                cout << curNode << " is not alive!" << endl;
                cout << checkNodes.size() << endl;
                handlePeerFail(curNode);
                it = checkNodes.erase(it);
            } else
                it++;
        }
    }
}

string Node::sendMessage(string ip, string port, string msg){

	int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    // struct hostent *server;
    struct timeval tv;

    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;


    char buffer[MAX];
    char recv_message[MAX];

    portno = atoi(port.c_str());
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    serv_addr.sin_port = htons(portno);
    // cout << "Connecting to " << ip+":"+port << endl;
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        //perror("ERROR connecting");
        return "disconnect";
    }

    memset(buffer, 0, MAX);
    strcpy(buffer,msg.c_str());
    cout << "Sending message " << msg << " to " << ip + "<" + port << endl;
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
        perror("ERROR writing to socket");

    memset(recv_message, 0, MAX);
    //read till timeout occurs/connection is broken
    while((n=read(sockfd,buffer,MAX)) > 0) {
        //cout << "Read " << n << " bytes of sendmessage reply" << endl;
        strncat(recv_message, buffer, n);
        memset(buffer, 0, MAX);
    }
    
    if(n < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            //cout << "timeout occurred" << endl;
            return "timeout";
        } else {
            perror("ERROR reading from socket");
        }
    }
    
    cout << "Got message " << recv_message << " from " << ip + "<" + port << endl;
    close(sockfd);
	return recv_message;
}

void Node::handlePeerFail(string failnodeid) {
    vector<Job>::iterator it;
    if(nodeToJobMap.find(failnodeid)==nodeToJobMap.end()) return;
    for(it=nodeToJobMap[failnodeid].begin();it!=nodeToJobMap[failnodeid].end();) {
        Job j=*it;
        char buf[MAX];
        unsigned long x = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        MD5 md5;
        strcpy(buf,j.execFile.c_str());
        strcat(buf, to_string(x).c_str());
        string exf(md5.digestString(buf));
        strcpy(buf,j.ipFile.c_str());
        strcat(buf, to_string(x).c_str());
        string ipf(md5.digestString(buf));
        string newjid=exf+string("<")+ipf;
        set<string>::iterator z=jobToNodeMap[j.jobId].begin();
        while(*z!=failnodeid) z++;
        parent[newjid]=j.jobId;
        submitJob(j.execFile,j.ipFile,newjid);
        it++;
    }
    nodeToJobMap.erase(failnodeid);
    return;
}

void Node::mergeResult(string nodeid,string jobid,string opfile) {
    //opfile=filerename(opfile);
    set<string>::iterator it=jobToNodeMap[jobid].begin();
    while(it!=jobToNodeMap[jobid].end() && *it!=nodeid) it++;
    if(it==jobToNodeMap[jobid].end()) return;
    result[jobid].insert(opfile);
    while(result[jobid].size()==jobToNodeMap[jobid].size()) {
        Application app;
        string of=app.merge(result[jobid]);
        result.erase(jobid);
        jobToNodeMap.erase(jobid);
        if(parent.find(jobid)!=parent.end()) {
            string parentJob=parent[jobid];
            result[parentJob].insert(of);
            parent.erase(jobid);
            jobid=parentJob;
        } else {
            cout<< "Output of "<<md5_original[jobid]<<" stored in "<< of<< endl;
            md5_original.erase(jobid);
            break;
        }
    }
    if(nodeid==ID) return;
    if(nodeToJobMap[nodeid].size()==1) {
        nodeToJobMap.erase(nodeid);
        checkNodes.erase(nodeid);
    } else {
        vector<Job>::iterator it=nodeToJobMap[nodeid].begin();
        while(it->jobId!=jobid) it++;
        nodeToJobMap[nodeid].erase(it);
    }
    return; 
}

void Node::receiveMessage(){
	int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[MAX];
    string replybuffer;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
	    perror("Server Socket ");

    int yesopt = 1;
    setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&yesopt,sizeof(int));

    portno = atoi(port.c_str());
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    	perror("bind()");
    cout << "Ready to receive any message" << endl;
    listen(sockfd,MAX_CONN);
	clilen = sizeof(cli_addr);
    while(1){
	    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    if (newsockfd < 0)
	        perror("accept()");
        replybuffer = "";
        memset(buffer, 0, MAX);
        int n = read(newsockfd,buffer,MAX);
	    if (n < 0) 
            perror("ERROR reading from socket");
	    if(buffer[0] == IAmUp+'0'){
            string str(buffer);
            int idx = str.find("::");
	    	//checkNodes.insert(str.substr(idx+2));
            //receive_IamUP(str.substr(idx+2));
            cout << "Peer connected: " << str.substr(idx+2) << endl;
            replybuffer += to_string(ReplyAlive);
            replybuffer += "::";
            replybuffer += ip;
            replybuffer += "<"+port;
	    }
        else if(buffer[0] == Mapping+'0'){
            string str(buffer);
            int idx = str.find("::");
            int idx1 = str.find(":",idx+2);
            int idx2 = str.find(":",idx1+1);
            int idx3 = str.find(":",idx2+1);
            int idx4 = str.find(":",idx3+1);
            string execName, ipName, jobId, ownerId;
            execName = str.substr(idx+2,idx1-idx-2);
            ipName = str.substr(idx1+1,idx2-idx1-1);
            jobId = str.substr(idx2+1,idx3-idx2-1);
            ownerId = str.substr(idx3+1,idx4-idx3-1);
            Job job;
            job.execFile = execName;
            job.ipFile = ipName;
            job.jobId = jobId;
            job.ownerId = ownerId;
            inputToJobMap[execName] = job;
            replybuffer += "Mapping Successful";
            cout << "inputToJobMap size: " << inputToJobMap.size() << endl;
            cout << execName << endl;
        }
        else if(buffer[0] == Query+'0'){
            int te = globalQ.size();
            replybuffer += to_string(te);
        }
        else if(buffer[0] == Result + '0'){
            string str(buffer);
            int idx = str.find("::");
            int idx1 = str.find(":",idx+2);
            int idx2 = str.find(":",idx1+1);
            int idx3 = str.find(":",idx2+1);
            string senderId = str.substr(idx+2,idx1-idx-2);
            string jobId = str.substr(idx1+1,idx2-idx1-1);
            string opFile = str.substr(idx2+1,idx3-idx2-1);

            //wait for opFile to become available
            unique_lock<mutex> lk{*opFileMutex};
            while(opFilesPending.find(opFile)==opFilesPending.end()) {
                opFileCond->wait(lk);
            }
            opFilesPending.erase(opFile);
            lk.unlock();
            mergeResult(senderId,jobId,opFile);
        } else if(buffer[0] == CheckAlive + '0') {
            replybuffer = "Alive!";
        }
	    //cout << "size of set is " << checkNodes.size() << endl;
	    n = write(newsockfd,replybuffer.c_str(),replybuffer.length());
	    if (n < 0) perror("ERROR writing to socket");
        close(newsockfd);
    }
    close(sockfd);
}

void Node::receiveFile(){
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[MAX];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("Server Socket ");

    int yesopt = 1;
    setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&yesopt,sizeof(int));

    portno = atoi((port).c_str())+1000;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        perror("bind()");
    cout << "Ready to receive any File" << endl;
    listen(sockfd,MAX_CONN);
    clilen = sizeof(cli_addr);
    while(1){
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            perror("accept()");

        string fileName = "";
        FILE *fp2;
        bool nameRead = false;
        while(1){
            memset(buffer,0,MAX);
            n = read(newsockfd,buffer,MAX);
            /**/
            if(n<0) {
                cout << "Error: execfile read failed." << endl;
                return;
            } else if(n==0) {
                break;
            }
            int i=0;
            for(i=0; !nameRead && i<n; i++) {
                if(buffer[i] == ':'){
                    //cout << "RECV FILE: " << fileName << endl;
                    fp2 = fopen(fileName.c_str(),"wb");
                    if(fp2==NULL) {
                        cout << "Error: invalid filename. " << errno << endl;
                        return;
                    }
                    memmove(buffer, buffer+i+1, n-i-1);
                    n = n-i-1;
                    nameRead = true;
                    break;
                }
                fileName += buffer[i];
            }
            if(n>0)
                fwrite(buffer,sizeof(char), n, fp2);
        }
        fclose(fp2);
        chmod(fileName.c_str(), S_IRWXU);
        close(newsockfd);
        
        // map<string,Job>::iterator it;
        // cout << inputToJobMap.count(fileName) << endl;
        // for(it = inputToJobMap.begin(); it != inputToJobMap.end(); it++){
        //     cout << fileName.size() <<","<< (it->first).size() << endl;
        // }
        cout << "Received a file with filename " << fileName << endl;
        unique_lock<mutex> lk{*opFileMutex};
        opFilesPending.insert(fileName);
        lk.unlock();
        (*opFileCond).notify_all();

        // cout << inputToJobMap.count(fileName) << endl;
        if(inputToJobMap.find(fileName) != inputToJobMap.end()){
            unique_lock<mutex> lk{*Qmutex};
            if(globalQ.empty()) {
                globalQ.push_back(inputToJobMap[fileName]);
                lk.unlock();
                Qnotempty->notify_all();
            } else {
                globalQ.push_back(inputToJobMap[fileName]);
                lk.unlock();
            }
            cout << "Job inserted in globalQ." << endl;
        }
    }
}

void Node::sendFile(string ip, string port, string srcFileName, string destFileName)
{
    if(destFileName.empty())
        destFileName = srcFileName;
    int fis_id,ps_id;
    struct sockaddr_in ps_addr,fis_addr;
    
    if((ps_id=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("Socket() for Peer Server");
        return;
    }
    
    ps_addr.sin_family=AF_INET;
    ps_addr.sin_addr.s_addr=inet_addr(ip.c_str());
    ps_addr.sin_port=htons(atoi(port.c_str())+1000);
    int ps_len=sizeof(ps_addr);

    if(connect(ps_id,(struct sockaddr *)&ps_addr,ps_len)==-1){
        perror("connect()");
        cout<<"Cannot connect to Peer Server\n";
        return;
    }
    
    FILE *fp=fopen(srcFileName.c_str(),"r");
    if(fp==NULL){
        cout << "Execfile open error: " << errno << endl;
        return;
    }
    fseek(fp,0,SEEK_END);
    int f_sz = ftell(fp);
    rewind(fp);
    int size = 0, nbytes = min(f_sz, MAX-1);

    char buffer[MAX];
    
    strcpy(buffer,(destFileName.c_str()));
    int sz = destFileName.size();
    buffer[sz++] = ':';
    write(ps_id,buffer,sz);
    while((size = fread(buffer,sizeof(char), nbytes, fp)) > 0)
    {
        write(ps_id, buffer, size);
        f_sz -= size;
        nbytes = min(f_sz, MAX-1);
    }
    fclose(fp);
    close(ps_id);
}

void Node::sendJobMapping(string ip, string port, string execFileName, string ipFileName, string jobId, string ownerId)
{
    char message[MAX];
    sprintf(message, "%d::%s:%s:%s:%s:", Mapping, execFileName.c_str(), ipFileName.c_str(), jobId.c_str(), ownerId.c_str());
    string ret = sendMessage(ip, port, message);
    return;
}