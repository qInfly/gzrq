
#include <srnethttp.h>
#include <srutils.h>
#include "srlogger.h"
#include "integrate.h"
using namespace std;


int Integrate::integrate(const SrAgent &agent, const string &srv,
                         const string &srt)
{
        SrNetHttp http(agent.server() + "/s", srv, agent.auth());
        if (0 != registerSrTemplate(http, xid, srt)) {	// Step 1,2,3
                srDebug("template regist failed");
                return -1;
        }

        http.clear();
        if (http.post("100," + agent.deviceID()) <= 0) {	// Step 4
                srDebug("get" + agent.deviceID() + "'s externalId failed");
                return -1;
        }
        SmartRest sr(http.response());
        SrRecord r = sr.next();

        if (r.size() && r[0].second == "50") { // Step 4: NO
                http.clear();
                if (http.post("101," + agent.deviceID()) <= 0) {	// Step 5
                        srDebug("add managedObjects failed");
                        return -1;
                }
                sr.reset(http.response());
                r = sr.next();
                if (r.size() == 3 && r[0].second == "501") {
                        id = r[2].second; // Step 7
                        string s = "102," + id + "," + agent.deviceID();
                        if (http.post(s) <= 0) {	// Step 8
                                srDebug("send a externalId failed");
                                return -1;
                        }
                        http.clear();
                        return 0;
                }
        } else if (r.size() == 3 && r[0].second == "500") { // Step 4: YES
                id = r[2].second;  // Step 6
                http.clear();
                return 0;
        }
        return -1;
}
