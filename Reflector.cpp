/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT                                             */
/*    FILE: Reflector.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "Reflector.h"
#include "base64_url.hpp"
#include "schema/key_schema.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/schema.h"
#include "rapidjson/error/en.h"
#include <iostream>
#include <streambuf>
#include <curl/curl.h>

using namespace std;
using namespace rapidjson;

//---------------------------------------------------------
// Constructor

Reflector::Reflector() {}

//---------------------------------------------------------
// Destructor

Reflector::~Reflector() {}

//---------------------------------------------------------
// Procedure: OnNewMail

bool Reflector::OnNewMail(MOOSMSG_LIST &NewMail) {
    MOOSMSG_LIST::iterator p;
    Document d;
    d.SetObject();
    using base64 = cppcodec::base64_url;

    for(p=NewMail.begin(); p!=NewMail.end(); p++) {
        CMOOSMsg &msg = *p;
        string key    = msg.GetKey();
        if (msg.IsDouble()) {
            Pointer(("/"+key).c_str()).Set(d, msg.GetDouble());
        } else if (msg.IsString()) {
            Pointer(("/"+key).c_str()).Set(d, msg.GetString().c_str());
        } else if (msg.IsBinary()) {
            auto data = msg.GetBinaryDataAsVector();
            Pointer(("/"+key).c_str()).Set(d, (BinMarker + base64::encode(data)).c_str());
        } else {
            Notify("REFLECTOR_RUN_WARNING", "Unhandled Mail: " + key);
        }
    }

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);

    return(pushData(string(buffer.GetString())));
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool Reflector::OnConnectToServer() {
    // register for variables here
    // possibly look at the mission file?
    // m_MissionReader.GetConfigurationParam("Name", <string>);
    // m_Comms.Register("VARNAME", 0);

    RegisterVariables();
    return(true);
}

//---------------------------------------------------------
// Procedure: Iterate()
//            happens AppTick times per second

bool Reflector::Iterate() {
    string data = pullData();
    Document d;
    using base64 = cppcodec::base64_url;
    if (d.Parse(data.c_str()).HasParseError()) {
        Notify("REFLECTOR_RUN_WARNING", string("Incoming data has JSON parse error ") + GetParseError_En(d.GetParseError()) + string(" at offset ") + to_string(d.GetErrorOffset()));
        Notify("REFLECTOR_RUN_WARNING", "Bad data: " + data);
        return false;
    }
    for (auto &m: d.GetObject()) {
        if (m.value.IsDouble()) {
            Notify(m.name.GetString(), m.value.GetDouble());
        } else if (m.value.IsString()) {
            string member = m.value.GetString();
            if (member.find(BinMarker) == 0) {
                auto bindata = base64::decode(member.erase(0, BinMarker.length()));
                Notify(m.name.GetString(), bindata);
            } else {
                Notify(m.name.GetString(), member);
            }
        }
    }
    return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Reflector::OnStartUp() {
    list<string> sParams;
    m_MissionReader.EnableVerbatimQuoting(false);
    if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
        list<string>::iterator p;
        for(p=sParams.begin(); p!=sParams.end(); p++) {
            string original_line = *p;
            string param = stripBlankEnds(toupper(biteString(*p, '=')));
            string value = stripBlankEnds(*p);

            if(param == "HOST") {
                reflectorHost = value;
            } else if(param == "TRANSMIT_ID") {
                txID = value;
            } else if(param == "RECEIVE_ID") {
                rxID = value;
            } else if(param == "KEY_FILE") {
                if (!loadKeys(value)) {
                    Notify("REFLECTOR_CONFIG_WARNING", "Unable to load keys from file " + value);
                }
            } else if(param == "VARIABLE") {
                txVars.push_back(value);
            } else {
                Notify("REFLECTOR_UNHANDLED_CONFIG_WARNING", original_line);
            }
        }
    } else {
        Notify("REFLECTOR_CONFIG_WARNING", "No config block found for " + GetAppName());
    }

    RegisterVariables();
    return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void Reflector::RegisterVariables() {
    for (auto &v: txVars) {
        Register(v);
    }
}

bool Reflector::loadKeys(string path) {
    std::ifstream infile;
    infile.open(path);
    // Make sure the file opened correctly
    if (!infile.is_open()) {
        cerr << "Failed to open configuration file " << path << endl;
        return (false);
    }
    // Vacuum up the key file
    std::string json;
    infile.seekg(0, ios::end);
    json.reserve(infile.tellg());
    infile.seekg(0, ios::beg);
    json.assign((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());
    // Load schema
    Document keys;
    Document keySchema;
    if (keySchema.Parse(reinterpret_cast<char*>(key_schema_json), key_schema_json_len).HasParseError()) {
        cerr << "JSON parse error " << GetParseError_En(keySchema.GetParseError());
        cerr << " in configuration schema at offset " << keySchema.GetErrorOffset() << endl;
        return (false);
    }
    // Parse and validate keys
    SchemaDocument keySchemaDoc(keySchema);
    SchemaValidator validator(keySchemaDoc);
    if (keys.Parse(json.c_str(), json.length()).HasParseError()) {
        cerr << "JSON parse error " << GetParseError_En(keys.GetParseError());
        cerr << " in configuration JSON at offset " << keys.GetErrorOffset() << endl;
        return (false);
    }
    if (!keys.Accept(validator)) {
        StringBuffer sb;
        validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
        cerr << "Invalid configuration schema: " << sb.GetString() << endl;
        cerr << "Invalid keyword: " << validator.GetInvalidSchemaKeyword() << endl;
        sb.Clear();
        validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
        cerr << "Invalid document: " << sb.GetString() << endl;
        return (false);
    }
    // Load keys into variables
    for (auto &key: keys.GetArray()) {
        if (key["id"].GetString() == txID) {
            txKey = key["key"].GetString();
        } else if (key["id"].GetString() == rxID) {
            rxKey = key["key"].GetString();
        }
    }
    return (true);
}

bool Reflector::pushData(string buf) {
    CURLcode ret;
    CURL *hnd;
    string response;
    char errbuf[CURL_ERROR_SIZE];
    struct curl_slist *slist1;

    slist1 = NULL;
    slist1 = curl_slist_append(slist1, ("X-Reflector-Key-Header: " + txKey).c_str());

    string request = reflectorHost + "/set/" + txID;

    hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_URL, request.c_str()); curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, buf.c_str());
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)buf.length());
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.52.1");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(hnd, CURLOPT_SSH_KNOWNHOSTS, "/home/moos/.ssh/known_hosts");
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    ret = curl_easy_perform(hnd);

    curl_easy_cleanup(hnd);
    hnd = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;

    return (bool)ret;

}

string Reflector::pullData() {
    CURLcode ret;
    CURL *hnd;
    string response;
    char errbuf[CURL_ERROR_SIZE];
    struct curl_slist *slist1;

    slist1 = NULL;
    slist1 = curl_slist_append(slist1, ("X-Reflector-Key-Header: " + rxKey).c_str());

    string request = reflectorHost + "/get/" + rxID;

    hnd = curl_easy_init();
    curl_easy_setopt(hnd, CURLOPT_URL, request.c_str());
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.38.0");
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_SSH_KNOWNHOSTS, "/home/debian/.ssh/known_hosts");
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(hnd, CURLOPT_FAILONERROR, 1);			// trigger a failure on a 400-series return code.
    curl_easy_setopt(hnd, CURLOPT_TIMEOUT_MS, 250);			// set the timeout to 250 ms
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);

    // set up data writer callback
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void *)&response);

    // make request
    ret = curl_easy_perform(hnd);

    // clean up request
    curl_easy_cleanup(hnd);
    hnd = NULL;
    curl_slist_free_all(slist1);
    slist1 = NULL;


    // return data
    return (response);
}

/// @ Data writer callback for libcurl. userp must be of type std::string, otherwise it's gonna be weird.
size_t Reflector::writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	string *target = (string *)userp;
	char *ptr = (char *)contents;

	//target->clear();
	size *= nmemb;
	target->reserve(size);
	for (size_t i = 0; i < size; i++) {
		*target += ptr[i];
	}
	return size;
}
