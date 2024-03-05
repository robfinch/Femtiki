#ifndef CONST_H
#define CONST_H

#define TRUE        1
#define FALSE       0

#define null        (void *)0
#define MAX_UINT    0xFFFFFFFFFFFFFFFF
#define MAX_INT			0x7FFFFFFFFFFFFFFF
#define TS_NONE     0
#define TS_TIMEOUT  1
#define TS_WAITMSG  2
#define TS_PREEMPT  4
#define TS_RUNNING  8
#define TS_READY   16

#define MQS_UNLIMITED    0
#define MQS_OLDEST       1
#define MQS_NEWEST       2

#define MBT_DATA         2
// message types
#define MT_NONE          0             // not a message
#define MT_FREE          1

enum(-1) {
     E_Ok = 0,
     E_BadTCBHandle,
     E_BadPriority,
     E_BadCallno,
     E_BadEntryno,
     E_Arg,
     E_BadASID,
     E_BadMapno,
     E_BadMbx,
     E_QueFull,
     E_NoThread,
     E_NotAlloc,
     E_NoMsg,
     E_Timeout,
     E_BadAlarm,
     E_NotOwner,
     E_QueStrategy,
     E_DCBInUse,
	 E_Busy,
	 E_BadPageno,
	 E_PagesizeMismatch,

     //; Device driver errors
     E_BadDevNum,// =	0x20,
     E_NoDev,
     E_BadDevOp,
     E_ReadError,
     E_WriteError,
     E_BadBlockNum,
     E_TooManyBlocks,

     // resource errors
     E_NoMoreMbx,// =	0x40,
     E_NoMoreMsgBlks,
     E_NoMoreAlarmBlks,
     E_NoMoreACBs,
     E_NoMoreTCBs,
     E_NoMem,
     E_TooManyTasks
};

#define OBJ_MAGIC	(('O' << 40) | ('B' << 32) | ('J' << 24) | ('E' << 16) | ('C' << 8) | 'T')
#define ACB_MAGIC	(('A' << 16) | ('C' << 8) | 'B')

#define OL_USER			0
#define OL_SUPERVISOR	1
#define OL_HYPERVISOR	2
#define OL_MACHINE		3

#define OBJ_WHITE		2
#define OBJ_GREY		1
#define OBJ_BLACK		0

#define MMU_RD	4
#define MMU_WR	2
#define MMU_EX	1

// alarm constants
#define ALM_FOREVER	-1		// repeat forever code

#endif
