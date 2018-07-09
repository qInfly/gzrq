
#ifndef __DEF_TYPE_H__
#define __DEF_TYPE_H__

#include <stdint.h>

/************rename built-in type**************/
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef bool Bool;

enum TF {
        FALSE,
        TRUE
};

enum OpsStatus {
	PENGIND,
	EXECUTING,
	SUCCESSFUL,
	FAILED
};
/**********************************************/

#endif
