#pragma once
#include "crawler.h"
#include "winsock.h"
#include "common.h"
#include "urlparser.h"

void printStats(Parameters p);

int main(int argc, char* argv[])
{
	Winsock::initialize();	// initialize 

	queue <string> *urlQueue = new queue<string>();
	unordered_set <string> *hostSet = new unordered_set<string>();
	unordered_set <DWORD> *ipSet = new unordered_set<DWORD>();

	int threadCount = atoi(argv[1]);
	cout << threadCount << endl;

	if (argc != 3 || threadCount == 0) {
		printf("usage: Assignment1.exe [# of threads] [txt file input]\n");
		printf(".....press any enter to exit.....\n");
		getchar();
		exit(1);
	}

	string textFile = argv[2];

	streamoff fileSize = 0;
	ifstream file(textFile, std::ifstream::binary);
	if (file.is_open()) {

		file.seekg(0, file.end);
		fileSize = file.tellg();
		file.seekg(0, file.beg);

		cout << "Opening File..." << endl;
		string url;
		while (getline(file, url)) {
			//printf("url: %s\n", url.c_str());	
			urlQueue->push(url);
		}

		printf("Opened %s with size %llu bytes.\n", textFile.c_str(), (long long)fileSize);
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
	p.num_DNSlookup = 0;
	p.total_links = 0;
	p.num_robots = 0;
	p.num_tasks = 0;
	p.num_uniquehost = 0;
	p.num_uniqueIP = 0;
	p.num_crawled = 0;
	p.extracted_url = 0;
	p.q_size = 0;
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
	printf("Terminating main(), completion time %d s\n", (timeGetTime() - t)/1000);

	printStats(p);

	Winsock::cleanUp();

	//printf("Enter any key to continue ...\n");
	//getchar();

	system("pause");

	return 0;   // 0 means successful
}

void printStats(Parameters p) {
	cout << "Extracted " << p.extracted_url << " URL @ /s"<<  endl;
	cout << "Looked up " << p.num_DNSlookup << " DNS names @ /s" << endl;
	cout << "Downloaded " << p.num_robots << " robots @ /s" << endl;
	cout << "Crawled " << p.num_crawled << " pages @ /s" << endl;
	cout << "Parsed " << p.total_links << " links @ /s" << endl;
	cout << "HTTP codes: 2xx = " << p.statusCodeCount[2] << ", 3xx = " << p.statusCodeCount[3] << ", 4xx = " 
		<< p.statusCodeCount[4] << ", 5xx = " << p.statusCodeCount[5] << ", other = " << p.statusCodeCount[1] << endl;
}