#pragma once

#include "common.h"
#include "urlparser.h"
#include "winsock.h"

int connectDownloadVerify(Winsock ws, DWORD ip, URLParser parser, bool header, string & reply);
int parseStatusCode(string reply);
void countLinks(string reply);

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
};

// this function is where the thread starts
static UINT thread(LPVOID pParam)
{
	Winsock::initialize();	// initialize 

	Parameters *p = ((Parameters*)pParam);

	bool ipUnique = false;
	string url = "", host = "", path = "", query = "", request = "", reply = "";
	int statusCode;
	short port = 80;
	DWORD ip = 0;
	Winsock ws;

	HANDLE	arr[] = { p->eventQuit, p->mutex };
	while (true)
	{
		if (WaitForMultipleObjects(2, arr, false, INFINITE) == WAIT_OBJECT_0) { // the eventQuit has been signaled 
			cout << "event quit has been signaled" << endl;
			break;
		}
		else
		{
			WaitForSingleObject(p->mutex, INFINITE);
			
			//cout << "signled" << endl;
			if (p->urlQueue->empty()) {
				SetEvent(p->eventQuit);
				break;
			}
			
			// obtain ownership of the mutex
			// ------------- entered the critical section ------------------
			url = p->urlQueue->front();
			p->urlQueue->pop();
			cout << "URL: " << url << endl;
			ReleaseMutex(p->mutex);		// return mutex
			// ------------- left the critical section ------------------		
			URLParser parser;
			parser.parse(url);
			host = parser.getHost();
			path = parser.getPath();
			query = parser.getQuery();
			port = parser.getPort();
			ipUnique = false;
			reply = "";

			cout << "	Parsing URL... host " << host << ", port " << port << ", path " << path << endl;

			// ----------- check for uniqueness -----------
			cout << "	Checking host uniqueness... ";
			if (p->hostSet->find(host) == p->hostSet->end()) {
				p->hostSet->insert(host);
				cout << "passed" << endl;
				ip = ws.getIPaddress(host);

				cout << "	Checking IP uniqueness... ";
				if (p->ipSet->find(ip) == p->ipSet->end()) {
					p->ipSet->insert(ip);
					cout << "passed" << endl;
					ipUnique = true;
				}
				else { // unique IP test failed
					cout << "failed" << endl;
					continue;
				}
			}
			else { // unique host test failed
				cout << "failed" << endl;
				continue;
			}
			if (ipUnique) {
				ws.createTCPSocket();
 				statusCode = connectDownloadVerify(ws, ip, parser, true, reply);
				if (statusCode == -1) {
					cout << "	Failed Verification" << endl;
					continue;
				}
				ws.closeSocket();
				if (statusCode / 100 != 2) {
					ws.createTCPSocket();
					statusCode = connectDownloadVerify(ws, ip, parser, false, reply);
					if (statusCode == -1) {
						cout << "	Failed Verification" << endl;
						continue;
					}
					ws.closeSocket();
					countLinks(reply);
				}
			}
		}
	}

	ReleaseSemaphore(p->finished, 1, NULL);
	Winsock::cleanUp();

	return 0;
}

int connectDownloadVerify(Winsock ws, DWORD ip, URLParser p, bool header, string & reply) {
	int statCode = 0;

	string request = "";
	if (header) {
		request = "HEAD /robots.txt HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + p.getHost() + "\nConnection: close" + "\n\n";
		cout << "	Connecting on robots...";
	}
	else {
		request = "GET " + p.getPath() + p.getQuery() + " HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + p.getHost() + "\nConnection: close" + "\n\n";
		cout << "	Connecting on page...";
	}

	ws.connectToServerIP(ip, p.getPort());

	if (ws.sendRequest(request)) {
		if (ws.receive(reply)) {
			statCode = parseStatusCode(reply);
			cout << "	Verifying header... status code " << statCode << endl;
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
	intStatusCode = stoi(statusCode);
	return intStatusCode;
}

//Count links on downloaded page
void countLinks(string reply) {
	int linkCount = 0;
	int position = 0;

	cout << "	Parsing page... ";
	clock_t timer = clock();

	while ((position = reply.find("href", position)) != string::npos) {
		linkCount++;
		position += 4;
	}

	printf("done in %d ms with %d links\n", (clock() - timer)/1000, linkCount);
}