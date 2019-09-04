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
	//string testString = "Hello"; 
	//update(testString);
	//cout << testString << endl; 

	Winsock::initialize();	// initialize 

	// parse url to get host name, port, path, and so on.
	//string url = "https://www.udayton.edu/apply/index.php?bruh.php";
	//string url = "https://yahoo.com";

	if (argc < 3) {
		printf("usage: Assignment1.exe [url]\n");
		printf(".....press any enter to exit.....\n");
		getchar();
		exit(1);
	}

	string url = argv[2];
	if ((url.find("https") != string::npos)){
		printf("invalid scheme: http requests");
		printf(".....press any enter to exit.....\n");
		getchar();
		exit(2);
	}

	URLParser parser(url);
	string host0 = parser.getHost();
	string path0 = parser.getPath();
	string query0 = parser.getQuery();
	short port0 = parser.getPort();
	short port = 80;

	if (port0 == 0) {
		port0 = port;
	}

	cout << "URL: " << url << endl;
	cout << "	Parsing URL... host " << host0 << ", port " << port0 << ", request " << path0 << query0 << endl;	

	// the following shows how to use winsock functions

	Winsock ws, ws2;
	string host = "www.google.com";

	string getRequest = "GET " + path0 + query0 + " HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + host0 + "\nConnection: close" + "\n\n";
	string headRequest = "HEAD /robots.txt HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + host0 + "\nConnection: close" + "\n\n";

	string reply = "";
	string reply2 = "";

	/* ------------ You may create many sockets, one for each URL ------------- */
	ws.createTCPSocket();
	DWORD ip = ws.getIPaddress(host0);
	ws.connectToServerIP(ip, port);
	// Your task: send a request from your computer
	if (ws.sendRequest(headRequest)){
		// Your task: receive a reply from the server
		ws.receive(reply);
		if (ws.parseStatusCode(reply) == "200") {
			cout << "Status Code: 200" << endl;
			cout << "\n-----------------------------\nrecieved: \n" << reply;
		}
		else {
			cout << "Status Code Not 200: Attempting GET request" << endl;
			ws2.createTCPSocket();
			ws2.connectToServerIP(ip, port);
			if (ws2.sendRequest(getRequest)) {
				ws2.receive(reply2);
				cout << "\n-----------------------------\nrecieved: \n" << reply2;
			}
		}	
	}

	ws.closeSocket();

	printf("-----------------\n");

	/*

	// thread handles are stored here; they can be used to check status of threads, or kill them
	HANDLE *ptrs = new HANDLE[THREAD_COUNT];
	Parameters p;
	int num_peers = 20;

	// create a mutex for accessing critical sections (including printf)
	p.mutex = CreateMutex(NULL, 0, NULL);

	// create a semaphore that check if a thread finishs its task
	p.finished = CreateSemaphore(NULL, 0, THREAD_COUNT, NULL);

	//	p.active_threads = 0;
	p.num_tasks = num_peers;

	// create a manual reset event to determine the termination condition is true
	p.eventQuit = CreateEvent(NULL, true, false, NULL);

	// create a semaphore to keep track of the number of items in the inputQ. The initial size of inputQ is num_peers
	p.semaQ = CreateSemaphore(NULL, num_peers, MAX_SEM_COUNT, NULL);

	// get current system time
	DWORD t = timeGetTime();

	for (int i = 0; i < THREAD_COUNT; ++i)
	{
		// structure p is the shared space between the threads		
		ptrs[i] = CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)thread, &p, 0, NULL);
	}
	printf("-----------created %d threads-----------\n", THREAD_COUNT);

	// make sure this main thread hangs here until the other two quit; otherwise, the program will terminate prematurely
	for (int i = 1; i <= THREAD_COUNT; ++i)
	{
		WaitForSingleObject(p.finished, INFINITE);
		printf("%d thread finished. main() function there--------------\n", i);
	}
	printf("Terminating main(), completion time %d ms\n", timeGetTime() - t);
	
	*/



	Winsock::cleanUp();

	printf("Enter any key to continue ...\n");
	getchar();

	return 0;   // 0 means successful
}