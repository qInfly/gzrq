
#ifndef __CPUINFO_H__
#define __CPUINFO_H__

#include "sragent.h"
#include "srtimer.h"

class CpuInfo: public SrTimerHandler
{
public:
        CpuInfo();
        virtual ~CpuInfo();
        virtual void operator()(SrTimer &timer, SrAgent &agent);
};

#endif // __CPUINFO_H__
