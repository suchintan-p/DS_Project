#ifndef NODE_H
#define NODE_H
#include "Application.h"
#define MAX 256
#define BACKLOG 10

class Node{
public:
	Node(string ip, string port);
	void startUp();
	void submitJob(string execFileName, string ipFileName, string jobid="");
	void checkAlive();
	string sendMessage(string ip, string port, string msg);
	void receiveMessage();
	void sendFile(string ip, string port, string srcFileName, string destFileName="");
	void receiveFile();
	void sendJobMapping(string ip, string port, string execFileName, string ipFileName, string jobId, string ownerId);
	void handlePeerFail(string failnodeid);
	deque<Job> globalQ;
	void mergeResult(string nodeid,string jobid,string opfile);
	void executeJob();
	condition_variable* Qnotempty;
	mutex* Qmutex;
	condition_variable* opFileCond;
	mutex* opFileMutex;
private:
	string ip,port,ID; // ID= ip+":"+port, jobID= exFile+":"+ipFile
	
	set<string> checkNodes; // Nodes that we need to send checkAlive to
	map<string, vector<Job> > nodeToJobMap; // mapping for nodeid to set of job
	set<string> opFilesPending;
	map<string, set<string>> jobToNodeMap; // mapping of jobId to nodeIds running it
	map<string,string> md5_original; //md5 Job to original file names in job
	vector<pair<string,int>> load; // info of #jobs in waiting Q per nodeID
	map<string, Job> inputToJobMap; // mapping of input file to Job
	map<string,string> parent; // point to parent job on same node
	map<string,set<string>> result; // map jobId to the generated partial output files
};

#endif
