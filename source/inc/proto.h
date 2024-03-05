#ifndef _PROTO_H
#define _PROTO_H

// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2024  Robert Finch, Waterloo
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
// proto.h
// Function prototypes.
//
// BSD 3-Clause License
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//                                                                          
// ============================================================================

// ACB functions
ACB *GetACBPtr();                   // get the ACB pointer of the running task
ACB *GetRunningACBPtr();
hACB GetAppHandle();

void FMTK_Reschedule();
int FMTK_AllocMbx(hMBX *phMbx);
int FMTK_FreeMbx(hMBX hMbx);
int FMTK_CheckMsg(hMBX hMbx, int *d1, int *d2, int *d3, int qrmv);
int FMTK_PeekMsg(uint hMbx, int *d1, int *d2, int *d3);
int FMTK_SendMsg(hMBX hMbx, int d1, int d2, int d3);	// blocking
int FMTK_PostMsg(hMBX hMbx, int d1, int d2, int d3);	// non-blocking
int FMTK_WaitMsg(hMBX hMbx, int *d1, int *d2, int *d3, int timelimit);
int FMTK_StartThread(byte *pCode, int stacksize, int *pStack, char *pCmd, int info);
int FMTK_StartApp(AppStartupRec *rec);
void RequestIOFocus(ACB *);

pascal int chkTCB(register TCB *p);
pascal int InsertIntoReadyList(register hTCB ht);
pascal int RemoveFromReadyList(register hTCB ht);
pascal int InsertIntoTimeoutList(register hTCB ht, register int to);
pascal int RemoveFromTimeoutList(register hTCB ht);
void DumpTaskList();

pascal void SetBound48(TCB *ps, TCB *pe, int algn);
pascal void SetBound49(ACB *ps, ACB *pe, int algn);
pascal void SetBound50(MBX *ps, MBX *pe, int algn);
pascal void SetBound51(MSG *ps, MSG *pe, int algn);

pascal void set_vector(register unsigned int, register unsigned int);
int getCPU();
int GetVecno();          // get the last interrupt vector number
void outb(register unsigned int, register int);
void outc(register unsigned int, register int);
void outh(register unsigned int, register int);
void outw(register unsigned int, register int);
pascal int LockSemaphore(register int *sema, register int retries);
naked inline void UnlockSemaphore(register int *sema) __attribute__(__no_temps)
{
	asm {
		sto	r0,[a0]
	}
}
naked inline void SetVBA(register int value)  __attribute__(__no_temps)
{
	asm {
		csrrw	r0,a0,0x0004
	}
}

pascal int LockSysSemaphore(register int retries);
pascal int LockIOFSemaphore(register int retries);
pascal int LockKbdSemaphore(register int retries);
naked inline void UnlockIOFSemaphore()
{
	__asm {
		ldi	a0,#8
		csrrc	r0,a0,12
	}
}

naked inline void UnlockKbdSemaphore()
{
	__asm {
		ldi	a0,16
		csrrc	r0,a0,12
	}
}


naked inline int GetImLevel()
{
	__asm {
		csrrd	a0,r0,0x3004
		lsr a0,a0,5
		and	a0,a0,7
	}
}

// Restoring the interrupt level does not have a ramp, because the level is
// being set back to enable interrupts, from a disabled state. Following the
// restore interupts are allowed to happen, we don't care if they do.

naked inline void RestoreImLevel(register int level)
{
	__asm {
		csrrd	t0,r0,0x3004
		and a0,a0,7
		ror t0,t0,5
		and t0,t0,-8
		or t0,t0,a0
		rol t0,t0,5
		csrrw	a0,t0,0x3004
	}
}

// The following causes a privilege violation if called from user mode
#define check_privilege() asm { }

// tasks
void FocusSwitcher();

naked inline void LEDS(register int val)  __attribute__(__no_temps)
{
  __asm {
    stt a0,LEDS
  }
}

extern int mmu_Alloc8kPage();
extern pascal void mmu_Free8kPage(register int pg);
extern int mmu_Alloc512kPage();
extern pascal void mmu_Free512kPage(int pg);
extern pascal void mmu_SetAccessKey(int mapno);
extern pascal int mmu_SetOperateKey(int mapno);
extern pascal void *mmu_alloc(int amt, int acr);
extern pascal void mmu_free(void *pmem);
extern pascal void mmu_SetMapEntry(register void *physptr, register int acr, register int entryno);
extern int mmu_AllocateMap();
extern pascal void mmu_FreeMap(register int mapno);
extern int *mmu_MapCardMemory();

#endif
