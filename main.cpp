#include "Node.h"
#define PARENT 1
#define CHILD 2

int main(int argc, char *argv[]){
	Node node = Node(argv[1],argv[2]);
	string str = argv[2];
	node.startUp();
	string execFile, inputFile;
	string choice, ipFileName, execFileName;

	while(1)
	{
		cout << "Do you want to submit job? (y/n) : ";
		cin >> choice;

		if(choice[0] == 'y')
		{
			cout << "Enter executable file: ";
			cin >> execFile;
			cout << "Enter input file: ";
			cin >> inputFile;

			node.submitJob(execFile, inputFile);
		}
		else if(choice[0] == 'n')
		{
			continue;
		}
		else
		{
			cout << "Enter y or n!!\n";
		}
	}
	
	return 0;
}