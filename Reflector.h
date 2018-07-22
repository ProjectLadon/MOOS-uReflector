/************************************************************/
/*    NAME:                                               */
/*    ORGN: MIT                                             */
/*    FILE: Reflector.h                                          */
/*    DATE:                                                 */
/************************************************************/

#ifndef Reflector_HEADER
#define Reflector_HEADER

#include "MOOS/libMOOS/MOOSLib.h"
#include <utility>
#include <string>
#include <chrono>
#include <cfloat>
#include <cmath>
#include <vector>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

using namespace std;

class Reflector : public CMOOSApp {
    public:
        Reflector();
        ~Reflector();

    protected: // Standard MOOSApp functions to overload
        bool OnNewMail(MOOSMSG_LIST &NewMail);
        bool Iterate();
        bool OnConnectToServer();
        bool OnStartUp();

    protected:
        void RegisterVariables();

    private: // Configuration variables
        string txID;
        string txKey;
        string rxID;
        string rxKey;
        vector<string> txVars;

    private: // State variables
};

#endif
