#include <string.h>
#include <string>
#include <iostream>
#include <srlogger.h>
#include "cpuinfo.h"
using namespace std;

static bool getSystemLoad(int& cpu_info)
{
        const char* p_file = "/proc/loadavg";
        char buf[64] = {0};
        FILE* fp = fopen(p_file, "r");
        char err[64] = {0};
        if (NULL == fp) {
                sprintf(err, "%s: %s", p_file, strerror(errno));
                srError(err);
                return false;
        }

        if (!fgets(buf, sizeof(buf)-1, fp)) {
                sprintf(err, "%s", strerror(errno));
                srError(string(__FILE__) + "," + to_string(__LINE__)
                        + " " + string(err));
        }

        fclose(fp);

        char* p = strstr(buf, " ");
        *p = '\0';
        cpu_info = int(atof(buf) * 100);

        return true;
}

CpuInfo::CpuInfo()
{

}

CpuInfo::~CpuInfo()
{

}

void CpuInfo::operator()(SrTimer &timer, SrAgent &agent)
{
	agent.send("107," + agent.ID());	
//	int cpu_info = 0;;
//	if (getSystemLoad(cpu_info))
//		agent.send("401," + agent.ID() + "," + to_string(cpu_info));
}
