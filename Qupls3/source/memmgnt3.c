// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2025  Robert Finch, Waterloo
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
//
// This source file is free software: you can redistribute it and/or modify 
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or     
// (at your option) any later version.                                      
//                                                                          
// This source file is distributed in the hope that it will be useful,      
// but WITHOUT ANY WARRANTY; without even the implied warranty of           
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            
// GNU General Public License for more details.                             
//                                                                          
// You should have received a copy of the GNU General Public License        
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    
//                                                                          
// ============================================================================
//
#include "../inc/config.h"
#include "../inc/const.h"
//#include "../inc/errno.h"
#include "../inc/types.h"

#define NPAGES	65536-1024		// 1024 OS pages
#define CARD_MEMORY		0xFFFFFFFFFFCE0000
#define IPT_MMU				0xFFFFFFFFFFDCD000
#define IPT
#define MMU				0xFFFFFFFFFFF40000
#define MMU_VADR	0xFFFFFFFFFFF4FF40
#define MMU_PADR	0xFFFFFFFFFFF4FF50
#define MMU_PADRV 0xFFFFFFFFFFF4FF60

#define NULL    (void *)0

extern byte *os_brk;

extern unsigned int lastSearchedPAMWord;
extern int errno;
extern __int8 *osmem;
extern int highest_data_word;
extern __int16 mmu_freelist;		// head of list of free pages
extern int syspages;
extern int sys_pages_available;
extern int mmu_FreeMaps;
extern int mmu_key;
extern __int8 hSearchMap;
extern MEMORY memoryList[NR_MEMORY];
extern int RTCBuf[12];
void puthexnum(int num, int wid, int ul, char padchar);
void putnum(int num, int wid, int sepchar, int padchar);
extern void DBGHideCursor(int hide);
extern void* memsetT(void* ptr, __int32 c, size_t n);
extern __int8* FindFreePage();
extern unsigned __int32* GetPageTableEntryAddress(unsigned __int32* virtadr);

//unsigned __int32 *mmu_entries;
extern unsigned __int32 PAM[(NPAGES+1)/32];
extern PMTE PageManagementTable[NPAGES];
extern unsigned __int32* page_table;
extern byte *brks[512];
extern unsigned __int32 pebble[512];

// Checkerboard ram test routine.

void ramtest()
{
	int *p;
	int errcount;
	
	errcount = 0;
	DBGHideCursor(1);
	DBGDisplayStringCRLF("Writing 5A code to ram");
	for (p = 0; p < 536870912; p += 2) {
		if ((p & 0xfffff)==0) {
			putnum(p>>20,5,',',' ');
			DBGDisplayChar('M');
			DBGDisplayChar('B');
			DBGDisplayChar('\r');
		}
		p[0] = 0x5555555555555555L;
		p[1] = 0xAAAAAAAAAAAAAAAAL;
	}
	DBGDisplayStringCRLF("\r\nReadback 5A code from ram");
	for (p = 0; p < 536870912; p += 2) {
		if ((p & 0xfffff)==0) {
			putnum(p>>20,5,',',' ');
			DBGDisplayChar('M');
			DBGDisplayChar('B');
			DBGDisplayChar('\r');
		}
		if (p[0] != 0x5555555555555555L || p[1] != 0xAAAAAAAAAAAAAAAAL) {
			errcount++;
			if (errcount > 10)
				break;
		}
	}
	DBGDisplayString("\r\nerrors: ");
	putnum(errcount,5,',',' ');
	errcount = 0;
	DBGDisplayStringCRLF("\r\nWriting A5 code to ram");
	for (p = 0; p < 536870912; p += 2) {
		if ((p & 0xfffff)==0) {
			putnum(p>>20,5,',',' ');
			DBGDisplayChar('M');
			DBGDisplayChar('B');
			DBGDisplayChar('\r');
		}
		p[0] = 0xAAAAAAAAAAAAAAAAL;
		p[1] = 0x5555555555555555L;
	}
	DBGDisplayStringCRLF("\r\nReadback A5 code from ram");
	for (p = 0; p < 536870912; p += 2) {
		if ((p & 0xfffff)==0) {
			putnum(p>>20,5,',',' ');
			DBGDisplayChar('M');
			DBGDisplayChar('B');
			DBGDisplayChar('\r');
		}
		if (p[1] != 0x5555555555555555L || p[0] != 0xAAAAAAAAAAAAAAAAL) {
			errcount++;
			if (errcount > 10)
				break;
		}
	}
	DBGDisplayString("\r\nerrors: ");
	putnum(errcount,5,',',' ');
	DBGDisplayChar('\r');
	DBGDisplayChar('\n');
	DBGHideCursor(0);
}


//private __int16 pam[NPAGES];	
// There are 128, 4MB pages in the system. Each 4MB page is composed of 64 64kb pages.
//private int pam4mb[NPAGES/64];	// 4MB page allocation map (bit for each 64k page)
//int syspages;					// number of pages reserved at the start for the system
//int sys_pages_available;	// number of available pages in the system
//int sys_4mbpages_available;

static unsigned int round16k(register unsigned int amt)
{
  amt += 16383;
  amt &= 0xFFFFFFFFFFFFC000L;
  return (amt);
}

// ----------------------------------------------------------------------------
// Must be called to initialize the memory system before any
// other calls to the memory system are made.
// Initialization includes setting up the linked list of free pages and
// setting up the 512k page bitmap.
// ----------------------------------------------------------------------------

void init_memory_management()
{
	unsigned __int32* pACB;

	// System break positions.
	// All breaks start out at address 16777216. Addresses before this are
	// reserved for the video frame buffer. This also allows a failed
	// allocation to return 0.
	DBGDisplayChar('A');
	sys_pages_available = NPAGES;
	lastSearchPAMWord = PAM;
  
  // Allocate 16MB to the OS, 8MB OS, 8MB video frame buffer
  osmem = pt_alloc(8388607,7);
  // Allocate video frame buffer
  pACB = GetRunningACBPtr();
  pACB[246] = pt_alloc(8388607,7);
	DBGDisplayChar('a');
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

unsigned __int32* GetPageTableAddr(byte* vadr)
{
	unsigned __int32* vadr_reg = MMU_VADR;
	unsigned __int32* padr_reg = MMU_PADR;
	unsigned __int32* padrv_reg = MMU_PADRV;
	
	GetPageTableAddress(vadr);
}

// ----------------------------------------------------------------------------
// Alloc enough pages to fill the requested amount.
// ----------------------------------------------------------------------------

void *pt_alloc(int amt, int acr)
{
	__int8 *p;
	int npages;
	int nn;
	byte* page;
	unsigned __int32* pe;
	unsigned __int32 en;
	unsigned __int32* ptcb;

	p = -1;
	DBGDisplayChar('B');
	amt = round16k(amt);
	npages = amt >> 14;
	if (npages==0)
		return (p);
	DBGDisplayChar('C');
	if (npages < sys_pages_available) {
		sys_pages_available -= npages;
		ptcb = GetRunningTCBPointer();
		p = ptcb[230];
		ptcb[230] += amt;
		for (nn = 0; nn < npages-1; nn++) {
			page = FindFreePage();
			pe = GetPageTableEntryAddress(page+(nn << 14));
			en = (acr << 13) | ((page >> 14) & 0x1fff) | 0x80000000;
			*pe = en;
		}
		page = FindFreePage();
		pe = GetPageTableEntryAddress(page+(nn << 14));
		en = (acr << 13) | ((page >> 14) & 0x1fff) | 0x80200000;
		*pe = en;
	}
//	p |= (asid << 52);
	DBGDisplayChar('E');
	return (p);
}


// ----------------------------------------------------------------------------
// pt_free() frees up 16kB blocks previously allocated with pt_alloc(), but does
// not reset the virtual address pointer. The freed blocks will be available for
// allocation. With a 64-bit pointer the virtual address can keep increasing with
// new allocations even after memory is freed.
//
// Parameters:
//		vadr - virtual address of memory to free
// ----------------------------------------------------------------------------

byte *pt_free(byte *vadr)
{
	int n;
	int count;	// prevent looping forever
	int vpageno;
	int last_page;
	unsigned __int32* pe;
	unsigned __int32 pte;

	count = 0;
	do {
		vpageno = (vadr >> 14) & 0xffff;
		while (LockMMUSemaphore(-1)==0);
		pe = GetPageTableEntryAddress(vadr);
		pte = *pe;
		last_page = (pte >> 22) & 1;
		while (LockPMTSemaphore(-1)==0);
		if (PageManagementTable[vpageno].share_count != 0) {
			PageManagementTable[vpageno].share_count--;
			if (PageManagementTable[vpageno].share_count==0) {
				pte = 0;
				*pe = pte;
			}
		}
		UnlockPMTSemaphore();
		UnlockMMUSemaphore();
		if (last_page)
			break;
		vadr += 16384;
		count++;
	}
	while (count < NPAGES);
	return (vadr);
}

// Returns:
//	-1 on error, otherwise previous program break.
//
void *sbrk(int size)
{
	byte *p, *q, *r;
	unsigned __int32 pte;
	unsigned __int32* ptcb;
	unsigned __int32* pe;

	p = 0;
	ptcb = GetRunningTCBPointer();
	size = round16k(size);
	if (size > 0) {
		p = pt_alloc(size,7);
		if (p==-1)
			errno = E_NoMem;
		return (p);
	}
	else if (size < 0) {
		size = -size;
		if (size > ptcb[230]) {
			errno = E_NoMem;
			return (-1);
		}
		r = p = ptcb[230] - size;
		for(q = p; q < ptcb[230];)
			q = pt_free(q);
		ptcb[230] = r;
		// Mark the last page
		if (r > 0) {
			while (LockMMUSemaphore(-1)==0);
			pe = GetPageTableEntryAddress(r);
			pte = *pe;
			pte &= 0xfcffffff;
			pte |= 0x00200000;
			*pe = pte;
			UnlockMMUSemaphore();
		}
	}
	else {	// size==0
		p = ptcb[230];
	}
	return (p);
}
