#pragma once
#include "common.h"

class URLParser {

public:

	// constructor 
	URLParser(string & link)  // & means pass the address of link to this function
	{
		url = link;
		host = "";
		port = 80;  // default port is 80 for web server
		path = "/";  // if path is empty, use "/"
		query = "";
	}

	/* url format:
	* scheme://[user:pass@]host[:port][/path][?query][#fragment]
	*/

	// e.g., url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// host: "cs.somepage.edu"
	string getHost()
	{
		// implement here, you may use url.find(...)

		removeHTTP(url);
		removeFragment(url);
		removeEndSlash(url);

		int colonPos = 0;
		int slashPos = 0;
		int queryPos = 0;

		if ((colonPos = url.find(':')) == -1) {
			if ((slashPos = url.find('/')) == -1) {
				if ((queryPos = url.find('?')) == -1) {
					host = url;
				}
				else{
					host = url.substr(0, queryPos);
				}
			}
			else {
				host = url.substr(0, slashPos);
			}
		}
		else {
			host = url.substr(0, colonPos);
		}

		return host;
	}

	// e.g., url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// port: 467
	short getPort()
	{
		string sPort = "";

		// implement here: find substring that represents the port number

		int colonPos = 0;
		int slashPos = 0;
		int queryPos = 0;

		if ((colonPos = url.find(":"))== -1) {
			return 80;
		}
		if ((slashPos = url.find('/')) == -1) {
			if ((queryPos = url.find('?')) == -1) {
				sPort = url.substr(colonPos);
			}
			else {
				sPort = url.substr(colonPos + 1, (queryPos - colonPos) - 1);
			}
		}
		else {
			sPort = url.substr(colonPos + 1, (slashPos - colonPos) - 1);
		}

		//cout << "Port: " << sPort << endl;

		if (sPort.length() > 0)
			port = atoi(sPort.c_str());  // convert substring sPort to an integer value

		return port;
	}

	// url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// path is "/index.php"
	string getPath()
	{
		// implement here

		int slashPos = 0;
		int queryPos = 0;
		

		if ((slashPos = url.find('/')) == -1) {
			path = "/";
		}
		else if((queryPos = url.find('?')) == -1){
			path = url.substr(slashPos + 1);
		}
		else {
			path = url.substr(slashPos, (queryPos - slashPos) - 1);
		}

		return path;
	}

	// url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// query is "?addrbook.php"
	string getQuery()
	{
		// implement here

		int queryPos = 0;

		if ((queryPos = url.find('?')) == -1) {
			query = "";
		}
		else {
			query = url.substr(queryPos);
		}

		return query;
	}


private:
	string url;
	string host;
	short port;
	string path;
	string query;

	void removeHTTP(string & url) {
		if (url.find("https://") != string::npos) {
			url.erase(0, 8);
		}
		else if (url.find("http://") != string::npos) {
			url.erase(0,7);
		}
	}

	void removeFragment(string & url) {
		int fragPos = 0;

		if ((fragPos = url.find("#")) != -1) {
			url.erase(fragPos, url.length());
		}
	}

	void removeEndSlash(string & url) {
		char endSlash = ' ';

		if (url[url.length() - 1] == '/') {
			url.erase(url.length() - 1);
		}

	}
};
