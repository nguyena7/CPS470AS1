#pragma once
#include "crawler.h"
#include "winsock.h"
#include "common.h"
#include "urlparser.h"

void printStats(Parameters p, int CompletionTime);

int main(int argc, char* argv[])
{

	queue <string> *urlQueue = new queue<string>();
	unordered_set <string> *hostSet = new unordered_set<string>();
	unordered_set <DWORD> *ipSet = new unordered_set<DWORD>();
	int compTime = 0;

	// --------- command line arguments ----------- //
	int threadCount = atoi(argv[1]);
	if (argc != 3 || threadCount == 0) {
		printf("usage: Assignment1.exe [# of threads] [txt file input]\n");
		printf(".....press any enter to exit.....\n");
		getchar();
		exit(1);
	}

	//----------------- Manage Input File -------------------//
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


	//----------------	Thead Creation/Management	-------------------------------------//

	// thread handles are stored here; they can be used to check status of threads, or kill them
	HANDLE *ptrs = new HANDLE[threadCount];

	Parameters p;

	// create a mutex for accessing critical sections (including printf)
	p.mutex = CreateMutex(NULL, 0, NULL);

	// create a semaphore that check if a thread finishs its task
	p.finished = CreateSemaphore(NULL, 0, threadCount, NULL);

	// Initialize Parameter variables
	p.hostSet = hostSet;
	p.ipSet = ipSet;
	p.urlQueue = urlQueue;
	p.remaining_url = urlQueue->size();
	p.thread_q = threadCount;

	// create a manual reset event to determine the termination condition is true
	p.eventQuit = CreateEvent(NULL, true, false, NULL);

	// get current system time
	DWORD t = timeGetTime();

	for (int i = 0; i < threadCount + 1; ++i)
	{
		// structure p is the shared space between the threads		
		ptrs[i] = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)thread, &p, 0, NULL);
	}
	printf("-----------created %d threads-----------\n", threadCount);

	// make sure this main thread hangs here until the other two quit; otherwise, the program will terminate prematurely
	for (int i = 1; i <= threadCount + 1; ++i)
	{
		WaitForSingleObject(p.finished, INFINITE);
		p.thread_q--;
	}
	compTime = timeGetTime() - t;
	printf("\nTerminating main(), completion time %d s\n", compTime/1000);

	printStats(p, compTime);

	system("pause");

	return 0;   // 0 means successful
}

void printStats(Parameters p, int CompletionTime) {
	printf("Extracted %d URLs @ %.2f/s\n", p.extracted_url, (p.extracted_url / ((CompletionTime) / 1000.0))); 
	printf("Looked up %d DNS names @ %.2f/s\n", p.num_DNSlookup, (p.num_DNSlookup / ((CompletionTime) / 1000.0)));
	printf("Downloaded %d robots @ %.2f/s\n", p.num_robots, (p.num_robots / ((CompletionTime) / 1000.0)));
	printf("Crawled %d pages @ %.2f/s (%.2fMB) \n", p.num_crawled, (p.num_crawled / ((CompletionTime) / 1000.0)), ((p.totalsize)/1000000.0));
	printf("Parsed %d links @ %.2f/s\n", p.total_links, (p.total_links / ((CompletionTime) / 1000.0)));
	cout << "HTTP codes: 2xx = " << p.statusCodeCount[2] << ", 3xx = " << p.statusCodeCount[3] << ", 4xx = " 
		<< p.statusCodeCount[4] << ", 5xx = " << p.statusCodeCount[5] << ", other = " << p.statusCodeCount[1] << endl;
}