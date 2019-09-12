#pragma once
#include "crawler.h"
#include "winsock.h"
#include "common.h"
#include "urlparser.h"

//void update(string  & s)
//{
//	s = ""; 
//}

int main(int argc, char* argv[])
{
	Winsock::initialize();	// initialize 

	queue <string> *urlQueue = new queue<string>();
	unordered_set <string> *hostSet = new unordered_set<string>();
	unordered_set <DWORD> *ipSet = new unordered_set<DWORD>();

	int threadCount = atoi(argv[1]);

	if (argc != 3 || threadCount != 1) {
		printf("usage: Assignment1.exe [# of threads] [txt file input]\n");
		printf(".....press any enter to exit.....\n");
		getchar();
		exit(1);
	}

	string textFile = argv[2];

	streamoff fileSize = 0;
	ifstream file(textFile, std::ifstream::binary);
	if (file.is_open()) {
		string url;
		while (getline(file, url)) {
			//printf("url: %s\n", url.c_str());
			fileSize = file.tellg();
			urlQueue->push(url);
		}
		cout << "Opened " << textFile << " with size " << fileSize << " bytes." << endl;
	}
	else {
		cout << "No such file." << endl;
		printf(".....press any enter to exit.....\n");
		getchar();
		exit(1);
	}

	file.close();

	// thread handles are stored here; they can be used to check status of threads, or kill them
	HANDLE *ptrs = new HANDLE[threadCount];
	Parameters p;
	//int num_peers = urlQueue->size();

	// create a mutex for accessing critical sections (including printf)
	p.mutex = CreateMutex(NULL, 0, NULL);

	// create a semaphore that check if a thread finishs its task
	p.finished = CreateSemaphore(NULL, 0, threadCount, NULL);

	//	p.active_threads = 0;
	//p.num_tasks = urlQueue.size();
	p.hostSet = hostSet;
	p.ipSet = ipSet;
	p.urlQueue = urlQueue;

	// create a manual reset event to determine the termination condition is true
	p.eventQuit = CreateEvent(NULL, true, false, NULL);

	// create a semaphore to keep track of the number of items in the inputQ. The initial size of inputQ is num_peers
	//p.semaQ = CreateSemaphore(NULL, num_peers, MAX_SEM_COUNT, NULL);

	// get current system time
	DWORD t = timeGetTime();

	for (int i = 0; i < threadCount; ++i)
	{
		// structure p is the shared space between the threads		
		ptrs[i] = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)thread, &p, 0, NULL);
	}
	printf("-----------created %d threads-----------\n", threadCount);

	// make sure this main thread hangs here until the other two quit; otherwise, the program will terminate prematurely
	
	for (int i = 1; i <= threadCount; ++i)
	{
		WaitForSingleObject(p.finished, INFINITE);
		printf("%d thread finished. main() function there--------------\n", i);
	}
	printf("Terminating main(), completion time %d ms\n", timeGetTime() - t);

	Winsock::cleanUp();

	printf("Enter any key to continue ...\n");
	getchar();

	return 0;   // 0 means successful
}
