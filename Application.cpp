#include "Application.h"

Application::Application(){ }

vector<Job> Application::split(Job job, int n){
	string in=job.ipFile, ex=job.execFile;
	MD5 md5;
    char buf[MAX];
	unsigned long t = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    strcpy(buf,in.c_str());
	strcat(buf, to_string(t).c_str());
    string ipf(md5.digestString(buf));
	vector<Job> ans;
	ifstream is;
	is.open(in.c_str());
	if(!is.is_open()) {
		cout << "Error opening input file: " << errno << endl;
		return ans;
	}
	int x;
	int ct=0;
	while(!is.eof()) {is>>x;ct++;}
	is.close();
	is.open(in.c_str());
	int p=(ct+n-1)/n;
	ofstream os;
	for(int i=1;i<=n;i++) {
		Job j;
		j.execFile=ex;
		stringstream ss;
		ss << i;
		j.ipFile=string("part")+ss.str()+string("_")+ipf;
		j.ownerId=job.ownerId;
		os.open(j.ipFile.c_str());
		int k=0;
		while(k<p && ct) {
			is>>x;
			os<<x<<endl;
			k++; ct--;
		}
		os.close();
    	j.jobId = job.jobId;
		ans.push_back(j);
		if(!ct) break;
	}
	for(int i=0;i<ans.size();i++) {
		ans[i].ipFile=filerename(ans[i].ipFile);
	}
	is.close();
	return ans;
}

string Application::merge(set<pair<int,string> > result){ // returns final o/p filename after merging
	static int ind=1;
	char t[20];
	sprintf(t,"file_%d.out",ind);
	ind++;
	ifstream is;
	int x,ans=0;
	set< pair<int,string> >::iterator it=result.begin();
	while(it!=result.end()) {
		is.open(it->second.c_str());
		is>>x;
		is.close();
		ans+=x;
		it++;
	}
	ofstream os;
	os.open(t);
	os<<ans<<endl;
	os.close();
	return string(t);
}