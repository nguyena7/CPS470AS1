#pragma once
#include "common.h"


class URLParser {

	/* url format:
	* scheme://[user:pass@]host[:port][/path][?query][#fragment]
	*/
public:
	// constructor, empty
	URLParser()
	{
	}

	void parse(string &url)
	{
		host = "";
		port = 80;  // default port is 80 for web server
		path = "/";  // if path is empty, use "/"
		query = "";

		// remove the line break if there is
		int pos = url.find("\n");
		if (pos != -1)
			url = url.substr(0, pos);

		// remove scheme, ://
		pos = url.find("://");
		if (pos != -1)
			url = url.substr(pos + 3);  // keep everything after ://
		// remove user:pass@
		pos = url.find("@");
		if (pos != -1)
			url = url.substr(pos + 1); // keep everyting after @
		// remove #fragment
		pos = url.find("#");
		if (pos != -1)
			url = url.substr(0, pos);  // remove fragment: keep everyting from index 0, span pos characters (does not include #)
		// find query
		pos = url.find("?");
		if (pos != -1)
		{
			query = url.substr(pos);
			url = url.substr(0, pos); // remove query: keep everyting from index 0, span pos characters (exclude ?)
		}
		// find path
		pos = url.find("/");
		if (pos != -1) {
			path = url.substr(pos);
			url = url.substr(0, pos);
		}
		// find port number
		pos = url.find(":");
		if (pos != -1) {
			port = stoi(url.substr(pos));
			url = url.substr(0, pos);
		}

		// what's left is host 
		host = url;
	}

	// call parse() before these get methods

	string getHost()
	{
		return host;
	}

	short getPort()
	{
		return port;
	}

	string getPath()
	{
		return path;
	}

	string getQuery()
	{
		return query;
	}

private:
	string host;
	short port;
	string path;
	string query;

};