
#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "def_type.h"

class Interface {
public:
        virtual ~Interface() {}

        virtual int getBuf(u8* w_buf) = 0;
        virtual int getBuf(u8* w_buf, const int& did) = 0;
};

#endif
