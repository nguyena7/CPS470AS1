#pragma once

#include "common.h"
#include "urlparser.h"
#include "winsock.h"

int connectDownloadVerify(Winsock ws, DWORD ip, string url, bool header, string & reply);
int parseStatusCode(string reply);

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
			URLParser parser(url);
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
 				statusCode = connectDownloadVerify(ws, ip, url, true, reply);
				if (statusCode == -1) {
					cout << "	Failed Verification" << endl;
					continue;
				}
				ws.closeSocket();
				if (statusCode / 100 != 2) {
					ws.createTCPSocket();
					statusCode = connectDownloadVerify(ws, ip, url, false, reply);
					if (statusCode == -1) {
						cout << "	Failed Verification" << endl;
						continue;
					}
					ws.closeSocket();
				}
			}
			if (reply.size() > 0) {
				cout << "	Will parse the reply later" << endl;
			}
				// delay here: contact a peer, send a request, and receive/parse the response 

				//cout << "URL taken from shared queue: " << url << endl;
				//Sleep(5000); // let this thread sleep for 5 seconds, just for code demonstration	
			
		} // end of while loop for this thread 
			// signal that this thread is exiting
	}

	ReleaseSemaphore(p->finished, 1, NULL);
	Winsock::cleanUp();

	return 0;
}

int connectDownloadVerify(Winsock ws, DWORD ip, string url, bool header, string & reply) {
	URLParser parser(url);
	int statCode = 0;

	string request = "";
	if (header) {
		request = "HEAD /robots.txt HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + parser.getHost() + "\nConnection: close" + "\n\n";
		cout << "	Connecting on robots...";
	}
	else {
		request = "GET " + parser.getPath() + parser.getQuery() + " HTTP/1.1\nUser-agent: UDCScrawler/1.0\nHost: " + parser.getHost() + "\nConnection: close" + "\n\n";
		cout << "	Connecting on page...";
	}

	ws.connectToServerIP(ip, parser.getPort());

	if (ws.sendRequest(request)) {
		if (ws.receive(reply)) {
			statCode = ws.parseStatusCode(reply);
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
