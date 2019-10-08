#pragma once

#include "common.h"
#include "urlparser.h"
#include "winsock.h"

int connectDownloadVerify(Winsock ws, DWORD ip, URLParser parser, bool header, string & reply, double maxDownloadSize); // Returns size of webpage downloaded
int parseStatusCode(string reply); //parses page reply for status code, returns 0 if failed.
int countLinks(string reply); // parse reply for links

// this class is passed to all threads, acts as shared memory
class Parameters {
public:
	HANDLE mutex;
	HANDLE finished;
	HANDLE eventQuit;
	HANDLE semaQ;
	queue <string> *urlQueue;
	unordered_set <string> *hostSet;
	unordered_set <DWORD> *ipSet;
	int num_uniquehost=0;			//H
	int num_DNSlookup=0;			//D
	int num_uniqueIP=0;				//I
	int num_robots=0;				//R
	int num_crawled=0;				//C
	int total_links=0;				//L
	int extracted_url=0;			//E
	int thread_q=0;					//Q
	int statusCodeCount[6] = {};	//used to count status codes
	double totalsize = 0;	//total size of downloaded content from webpages
	double temp_size = 0;	//size of current pages downloaded for displaying Mbps
	int remaining_url=0;	//remaining urls in Q
	int countingThread = 0; // First Thread will display data every 2 seconds
	int temp_crawled = 0;	//num of pages crawled for displaying Pages Per Second
	
};

void displayData(Parameters *p); // first thread spawned will run this function

// this function is where the thread starts
static UINT thread(LPVOID pParam)
{
	Winsock::initialize();	// initialize 

	Parameters *p = ((Parameters*)pParam);

	bool ipUnique, hostUnique;
	string url = "", host = "", path = "", query = "", request = "", reply = "";
	int statusCode;
	short port = 80;
	DWORD ip = 0;
	Winsock ws;
	URLParser parser;
	int firstDigitStatusCode = 0;
	int num_H, num_D, num_I, num_R, num_C, num_L, num_Q=0;					
	double pageSize;

	//select thread for displaying data every 2 seconds
	WaitForSingleObject(p->mutex, INFINITE);
	if (p->countingThread == 0) {
		p->countingThread++;
		ReleaseMutex(p->mutex);
		displayData(p);
		Winsock::cleanUp();
		return 0;
	}
	ReleaseMutex(p->mutex);

	while (true)
	{
		num_H = 0;			//H
		num_D = 0;			//D
		num_I = 0;			//I
		num_R = 0;			//R
		num_C = 0;			//C
		num_L = 0;			//L
		firstDigitStatusCode = 0;
		statusCode = 0;
		pageSize = 0;
		ipUnique = false;
		hostUnique = false;

		// obtain ownership of the mutex
		WaitForSingleObject(p->mutex, INFINITE);
		if (p->urlQueue->empty()) {
			ReleaseMutex(p->mutex);
			break;
		}

		// ------------- entered the critical section ------------------
		url = p->urlQueue->front();
		p->urlQueue->pop();
		p->remaining_url--;
		p->extracted_url++;
		ReleaseMutex(p->mutex);		// return mutex
		// ------------- left the critical section ------------------	

		//Parse URL
		parser.parse(url);
		host = parser.getHost();
		path = parser.getPath();
		query = parser.getQuery();
		port = parser.getPort();
		ipUnique = false;
		hostUnique = false;
		reply = "";

		// ----------- check for uniqueness -----------
		WaitForSingleObject(p->mutex, INFINITE);
		if (p->hostSet->find(host) == p->hostSet->end()) {		
			p->hostSet->insert(host);
			hostUnique = true;
			num_H++;	
		}
		ReleaseMutex(p->mutex);		// return mutex

		if (hostUnique) { //host name uniqueness
			ip = ws.getIPaddress(host);
			if (ip != INADDR_NONE) {
				num_D++;
			}

			WaitForSingleObject(p->mutex, INFINITE);
			if (p->ipSet->find(ip) == p->ipSet->end()) {	 //ip uniqueness			
				p->ipSet->insert(ip);
				num_I++;
				ipUnique = true;
			}
			ReleaseMutex(p->mutex);		// return mutex
		}

		if (ipUnique) { // passed IP unique test, attempting connection
			ws.createTCPSocket();
			pageSize += (double) connectDownloadVerify(ws, ip, parser, true, reply, 16000); 
			statusCode = parseStatusCode(reply);
			ws.closeSocket();

			if (statusCode != 0) {
				num_R++;
			}

			if (statusCode / 100 == 4) { // if robots is 4xx, crawl page
				ws.createTCPSocket();
				pageSize += (double) connectDownloadVerify(ws, ip, parser, false, reply, 1.6e+7);
				statusCode = parseStatusCode(reply);
				ws.closeSocket();
				if (statusCode != 0) {
					num_C++;
					num_L = countLinks(reply);
				}
			}
			firstDigitStatusCode = statusCode / 100;
		}

		
		// ------------ Update Parameters ------------------ //
		WaitForSingleObject(p->mutex, INFINITE);
		// ------------- entered the critical section ------------------
		p->statusCodeCount[firstDigitStatusCode]++;
		p->num_uniquehost += num_H;
		p->num_DNSlookup += num_D;
		p->num_uniqueIP += num_I;
		p->num_robots += num_R;
		p->num_crawled += num_C;
		p->total_links += num_L;
		p->totalsize += pageSize;
		p->temp_crawled += num_C;
		p->temp_size += pageSize;

		ReleaseMutex(p->mutex);		// return mutex
		// ------------- left the critical section ------------------

	}

	ReleaseSemaphore(p->finished, 1, NULL);
	Winsock::cleanUp();

	return 0;
}

int connectDownloadVerify(Winsock ws, DWORD ip, URLParser parser, bool header, string & reply, double maxDownloadSize) {
	int pageSize = 0;

	string request = "";
	if (header) { // header = true will send a head request
		request = "HEAD /robots.txt HTTP/1.0\nUser-agent: UDCScrawler/1.0\nHost: " + parser.getHost() + "\nConnection: close" + "\n\n";
	}
	else {
		request = "GET " + parser.getPath() + parser.getQuery() + " HTTP/1.0\nUser-agent: UDCScrawler/1.0\nHost: " + parser.getHost() + "\nConnection: close" + "\n\n";
	}

	ws.connectToServerIP(ip, parser.getPort());

	if (ws.sendRequest(request)) {
		pageSize = ws.receive(reply, maxDownloadSize);
	}
	return pageSize;
}
int parseStatusCode(string reply) {
	string statusCode = "";
	int intStatusCode = 0;

	string tempStr = reply;

	tempStr = tempStr.erase(0, 9);

	statusCode = tempStr.substr(0, 3);

	try {
		intStatusCode = stoi(statusCode);
	}
	catch(invalid_argument){
		return 0; // failed to convert status code
	}
	
	return intStatusCode;
}

//Count links on downloaded page
int countLinks(string reply) {
	int linkCount = 0;
	int position = 0;

	clock_t timer = clock();

	while ((position = reply.find("href", position)) != string::npos) {
		linkCount++;
		position += 4;
	}

	return linkCount;
}

void displayData(Parameters *p) { // the first thread that runs will display data every two seconds
	int num_Q = 0, num_E = 0, num_H = 0, num_D = 0, num_I = 0, num_R = 0, num_C = 0, num_L = 0, timer = 0, completedTime = 0, num_threads = 0;
	double pageSize = 0;
	while (true) {
		DWORD t = timeGetTime();
		if (p->thread_q == 0) {
			ReleaseSemaphore(p->finished, 1, NULL);
			break;
		}

		Sleep(2000);

		WaitForSingleObject(p->mutex, INFINITE);

		num_threads = p->thread_q;
		num_Q = p->urlQueue->size();
		num_E = p->extracted_url;
		num_H = p->num_uniquehost;
		num_D = p->num_DNSlookup;
		num_I = p->num_uniqueIP;
		num_R = p->num_robots;
		num_C = p->num_crawled;
		num_L = p->total_links;

		if (num_Q == -1)
			num_Q = 0;

		completedTime = timeGetTime() - t;
		timer += 2;
		printf("[%d] %d	Q %d	E %d	H %d	D %d	I %d	R %d	C %d	L %dk\n", timer, num_threads, num_Q, num_E, num_H, num_D, num_I, num_R, num_C, num_L/1000);
		printf("	*** crawling %.1f pps @ %.1f Mbps\n", p->temp_crawled / (completedTime / 1000.0), ((((p->temp_size))*8)/100000) / (completedTime / 1000.0));

		p->temp_crawled = 0;
		p->temp_size = 0;

		ReleaseMutex(p->mutex);
	}
}
