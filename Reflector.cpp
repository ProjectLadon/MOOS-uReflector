/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT                                             */
/*    FILE: Reflector.cpp                                        */
/*    DATE:                                                 */
/************************************************************/

#include <iterator>
#include "MBUtils.h"
#include "Reflector.h"

using namespace std;
using namespace rapidjson;

//---------------------------------------------------------
// Constructor

Reflector::Reflector()
{
}

//---------------------------------------------------------
// Destructor

Reflector::~Reflector()
{
}

//---------------------------------------------------------
// Procedure: OnNewMail

bool Reflector::OnNewMail(MOOSMSG_LIST &NewMail) {
    MOOSMSG_LIST::iterator p;
    Document d;
    d.SetObject()

    for(p=NewMail.begin(); p!=NewMail.end(); p++) {
        CMOOSMsg &msg = *p;
        string key    = msg.GetKey();
        if (msg.IsDouble()) {
            Pointer("/"+key).Set(d, msg.GetDouble());
        } else if (msg.IsString()) {
            Pointer("/"+key).Set(d, msg.GetString());
        } else if (msg.IsBinary()) {
            auto data = GetBinaryDataAsVector();
            
        } else {
            reportRunWarning("Unhandled Mail: " + key);
        }
    }

    return(true);

#if 0 // Keep these around just for template
    string key   = msg.GetKey();
    string comm  = msg.GetCommunity();
    double dval  = msg.GetDouble();
    string sval  = msg.GetString();
    string msrc  = msg.GetSource();
    double mtime = msg.GetTime();
    bool   mdbl  = msg.IsDouble();
    bool   mstr  = msg.IsString();
#endif

    return(true);
}

//---------------------------------------------------------
// Procedure: OnConnectToServer

bool Reflector::OnConnectToServer()
{
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

bool Reflector::Iterate()
{
  return(true);
}

//---------------------------------------------------------
// Procedure: OnStartUp()
//            happens before connection is open

bool Reflector::OnStartUp()
{
  list<string> sParams;
  m_MissionReader.EnableVerbatimQuoting(false);
  if(m_MissionReader.GetConfiguration(GetAppName(), sParams)) {
    list<string>::iterator p;
    for(p=sParams.begin(); p!=sParams.end(); p++) {
      string original_line = *p;
      string param = stripBlankEnds(toupper(biteString(*p, '=')));
      string value = stripBlankEnds(*p);

      if(param == "FOO") {
        //handled
      }
      else if(param == "BAR") {
        //handled
      }
    }
  }

  RegisterVariables();
  return(true);
}

//---------------------------------------------------------
// Procedure: RegisterVariables

void Reflector::RegisterVariables()
{
  // Register("FOOBAR", 0);
}
