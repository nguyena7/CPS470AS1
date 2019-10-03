#pragma once

#include "common.h"
#include "urlparser.h"
#include "winsock.h"

int connectDownloadVerify(Winsock ws, DWORD ip, URLParser parser, bool header, string & reply, double maxDownloadSize);
int parseStatusCode(string reply);
int countLinks(string reply);

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
	int num_tasks;
	int num_uniquehost;	//H
	int num_DNSlookup;	//D
	int num_uniqueIP;	//I
	int num_robots;		//R
	int num_crawled;	//C
	int total_links;	//L
	int extracted_url;	//E
	int q_size;			//Q
	int statusCodeCount[6] = {};
	int count = 0;
};

// this function is where the thread starts
static UINT thread(LPVOID pParam)
{
	Winsock::initialize();	// initialize 

	Parameters *p = ((Parameters*)pParam);

	bool ipUnique = false, hostUnique = false;
	string url = "", host = "", path = "", query = "", request = "", reply = "";
	int statusCode;
	short port = 80;
	DWORD ip = 0;
	Winsock ws;
	URLParser parser;
	int firstDigitStatusCode = 0;
	int count_200 = 0;
	int count_300 = 0;
	int count_400 = 0;
	int count_500 = 0;
	int other_status = 0;
	int num_H;			//H
	int num_D;			//D
	int num_I;			//I
	int num_R;			//R
	int num_C;			//C
	int num_L;			//L
	int num_E;			//E
	int num_Q = 0;		//Q

	HANDLE	arr[] = { p->eventQuit, p->mutex };
	while (true)
	{
		num_H = 0;			//H
		num_D = 0;			//D
		num_I = 0;			//I
		num_R = 0;			//R
		num_C = 0;			//C
		num_L = 0;			//L
		num_E = 0;			//E
		firstDigitStatusCode = 0;

		/*if (WaitForMultipleObjects(2, arr, false, INFINITE) == WAIT_OBJECT_0) { // the eventQuit has been signaled 
			cout << "event quit has been signaled" << endl;
			break;
		}
		else
		{*/
			WaitForSingleObject(p->mutex, INFINITE);
			
			//cout << "signled" << endl;
			if (p->urlQueue->empty()) {
				ReleaseMutex(p->mutex);
				//SetEvent(p->eventQuit);
				break;
			}
			
			// obtain ownership of the mutex
			// ------------- entered the critical section ------------------
			url = p->urlQueue->front();
			p->urlQueue->pop();
			p->count++;
			cout << "count: " << p->count << " ";
			cout << "URL: " << url << endl;
			num_E++; //E
			p->extracted_url++;
			//cout << "Extracted " << p->extracted_url << endl;
			ReleaseMutex(p->mutex);		// return mutex
			// ------------- left the critical section ------------------	

			parser.parse(url);
			host = parser.getHost();
			path = parser.getPath();
			query = parser.getQuery();
			port = parser.getPort();
			ipUnique = false;
			hostUnique = false;
			reply = "";

			//cout << "	Parsing URL... host " << host << ", port " << port << ", path " << path << endl;

			//create a separate function to determine host and ip uniquness. this function will be in critical section.
			// ----------- check for uniqueness -----------
			//cout << "	Checking host uniqueness... ";
			WaitForSingleObject(p->mutex, INFINITE);
			if (p->hostSet->find(host) == p->hostSet->end()) {		
				p->hostSet->insert(host);
				hostUnique = true;
				//cout << "passed" << endl;
				num_H++; //H	
			}
			ReleaseMutex(p->mutex);		// return mutex

			if (hostUnique) {
				ip = ws.getIPaddress(host);
				if (ip != INADDR_NONE) {
					num_D++; //D
				}		

				WaitForSingleObject(p->mutex, INFINITE);
				//cout << "	Checking IP uniqueness... ";
				if (p->ipSet->find(ip) == p->ipSet->end()) {				
					p->ipSet->insert(ip);
					num_I++; //I
					//cout << "passed" << endl;
					ipUnique = true;
				}
				ReleaseMutex(p->mutex);		// return mutex
			}

			if (ipUnique) {
				ws.createTCPSocket();
				statusCode = connectDownloadVerify(ws, ip, parser, true, reply, 16000);
				if (statusCode == -1) {
					//cout << "	Failed Verification" << endl;
					continue;
				}
				ws.closeSocket();
				if (statusCode / 100 != 2) {
					ws.createTCPSocket();
					statusCode = connectDownloadVerify(ws, ip, parser, false, reply, 1.6e+7);
					if (statusCode == -1) {
						//cout << "	Failed Verification" << endl;
						continue;
					}
					ws.closeSocket();
					num_C++;
					num_L = countLinks(reply);
				}
				else {
					num_R++;
				}
				firstDigitStatusCode = statusCode / 100;
				//cout << "statcode: " << firstDigitStatusCode << endl;
			}

			WaitForSingleObject(p->mutex, INFINITE);
			// obtain ownership of the mutex
			// ------------- entered the critical section ------------------
			p->statusCodeCount[firstDigitStatusCode]++;
			//cout << firstDigitStatusCode << ": " << p->statusCodeCount[firstDigitStatusCode] << endl;
			p->q_size = p->urlQueue->size();
			p->num_uniquehost += num_H;
			p->num_DNSlookup += num_D;
			p->num_uniqueIP += num_I;
			p->num_robots += num_R;
			p->num_crawled += num_C;
			p->total_links += num_L;

			ReleaseMutex(p->mutex);		// return mutex
			// ------------- left the critical section ------------------
		//}
	}

	ReleaseSemaphore(p->finished, 1, NULL);
	Winsock::cleanUp();

	return 0;
}

int connectDownloadVerify(Winsock ws, DWORD ip, URLParser parser, bool header, string & reply, double maxDownloadSize) {
	int statCode = 0;

	string request = "";
	if (header) {
		request = "HEAD /robots.txt HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + parser.getHost() + "\nConnection: close" + "\n\n";
		//cout << "	Connecting on robots...";
	}
	else {
		request = "GET " + parser.getPath() + parser.getQuery() + " HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + parser.getHost() + "\nConnection: close" + "\n\n";
		//cout << "	Connecting on page...";
	}

	ws.connectToServerIP(ip, parser.getPort());

	if (ws.sendRequest(request)) {
		if (ws.receive(reply, maxDownloadSize)) {
			statCode = parseStatusCode(reply);
			if (statCode == 0) {
				return -1;
			}
			//cout << "	Verifying header... status code " << statCode << endl;
		}
		else {
			return -1;
		}
	}
	else {
		return -1;
	}
	return statCode;
}

//parse for Status Code
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
		//cout << "Conversion Failed, No Status Code" << endl;
		return 0;
	}
	
	return intStatusCode;
}

//Count links on downloaded page
int countLinks(string reply) {
	int linkCount = 0;
	int position = 0;

	//cout << "	Parsing page... ";
	clock_t timer = clock();

	while ((position = reply.find("href", position)) != string::npos) {
		linkCount++;
		position += 4;
	}

	//printf("done in %d ms with %d links\n", (clock() - timer)/1000, linkCount);
	return linkCount;
}

