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
#include "../inc/const.h"
#include "../inc/config.h"
#include "../inc/errno.h"
#include "./kernel/types.h"

// There are 1024 pages in each map. In the normal 8k page size that means a max of
// 8Mib of memory for an app. Since there is 512MB ram in the system that equates
// to 65536 x 8k pages.
// However the last 144kB (18 pages) are reserved for memory management software.
#define NPAGES	65536
#define CARD_MEMORY		0xFFFFFFFFFFCE0000
#define IPT_MMU				0xFFFFFFFFFFDCD000
#define IPT

#define NULL    (void *)0

extern byte *os_brk;

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

//unsigned __int32 *mmu_entries;
extern unsigned __int32* page_table;
extern PMTE pmt[NPAGES];
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
	// System break positions.
	// All breaks start out at address 16777216. Addresses before this are
	// reserved for the video frame buffer. This also allows a failed
	// allocation to return 0.
	DBGDisplayChar('A');
	mmu_key = RTCBuf[0];	
	memsetT(brks,16777216,512);
	sys_pages_available = NPAGES;
  
  // Allocate 16MB to the OS
  osmem = pt_alloc(0,16777215,7);
	DBGDisplayChar('a');
}

// ----------------------------------------------------------------------------
// Allocate a single page.
// 
// ----------------------------------------------------------------------------

static void pt_alloc_page(unsigned int pbl, byte *vadr, int acr, int last_page)
{
	int i;
	unsigned __int32 vpageno;
	unsigned __int32 basepg;
	static int last_searched_page = 0;
	int page_count;

	basepg = (pbl & 0x65535);
	vpageno = ((vadr >> 14) & 0xffff) + basepg;
	for (i = last_searched_page, page_count = 0; page_count < NPAGES; page_count++) {
		while (LockMMUSemaphore()==0);
		if (pmt[i].share_count==0) {
			pmt[i].share_count = 1;
			// Mark page valid, set acr and physical page number
			page_table[vpageno] = (i & 0xffff)|((acr & 15) << 16)|0x80000000|(last_page<<22);
			UnlockMMUSemaphore();
			break;
		}
		UnlockMMUSemaphore();
		i++;
		if (i >= NPAGES)
			i = 0;
	}
	last_searched_page = i;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void *pt_alloc(int asid, int amt, int acr)
{
	__int8 *p;
	int npages;
	int nn;
	unsigned __int32 pbl;

	if (asid < 0 || asid > 511)
		throw (E_BadASID);
	p = -1;
	DBGDisplayChar('B');
	amt = round16k(amt);
	npages = amt >> 14;
	if (npages==0)
		return (p);
	DBGDisplayChar('C');
	pbl = pebble[asid];
	if (npages < sys_pages_available) {
		sys_pages_available -= npages;
		p = brks[asid];
		brks[asid] += amt;
		for (nn = 0; nn < npages-1; nn++) {
			pt_alloc_page(pbl,p+(nn << 14),acr,0);
		}
		pt_alloc_page(pbl,p+(nn << 14),acr,1);
	}
	p |= (asid << 52);
	DBGDisplayChar('E');
	return (p);
}


// pt_free() frees up 16kB blocks previously allocated with pt_alloc(), but does
// not reset the virtual address pointer. The freed blocks will be available for
// allocation. With a 64-bit pointer the virtual address can keep increasing with
// new allocations even after memory is freed.

byte *pt_free(byte *vadr)
{
	int n;
	int count;	// prevent looping forever
	int asid;
	int pbl;
	int vpageno;
	unsigned __int32 basepg;
	int last_page;

	asid = vadr >> 52;
	pbl = pebble[asid];
	vadr &= 0xFFFFFFFFFFFFFL;	// strip off asid
	count = 0;
	basepg = (pbl & 0x65535);
	while (LockMMUSemaphore()==0);
	do {
		vpageno = ((vadr >> 14) & 0xffff) + basepg;
		last_page = (page_table[vpageno] >> 24) & 1;
		if (pmt[vpageno].share_count != 0) {
			pmt[vpageno].share_count--;
			if (pmt[vpageno].share_count==0)
				page_table[vpageno] = 0;
		if (last_page)
			break;
		vadr += 16384;
		count++;
	}
	while (count < NPAGES);
	UnlockMMUSemaphore();
	return (vadr);
}

int mmu_AllocateMap()
{
	int count;
	
	for (count = 0; count < NR_MAPS; count++) {
		if (((mmu_FreeMaps >> hSearchMap) & 1)==0) {
			mmu_FreeMaps |= (1 << hSearchMap);
			return (hSearchMap);
		}
		hSearchMap++;
		if (hSearchMap >= NR_MAPS)
			hSearchMap = 0;
	}
	throw (errno = E_NoMoreACBs);
}

pascal void mmu_FreeMap(register int mapno)
{
	if (mapno < 0 || mapno >= NR_MAPS)
		throw (errno = E_BadMapno);
	mmu_FreeMaps &= ~(1 << mapno);	
}

// Returns:
//	-1 on error, otherwise previous program break.
//
void *sbrk(int size)
{
	byte *p, *q, *r;
	int as,oas;
	int pbl;
	int vpageno;
	unsigned __int32 basepg;
	unsigned __int32 pte;

	p = 0;
	size = round16k(size);
	if (size > 0) {
		as = GetASID();
		SetASID(0);
		p = pt_alloc(as,size,7);
		SetASID(as);
		if (p==-1)
			errno = E_NoMem;
		return (p);
	}
	else if (size < 0) {
		size = -size;
		as = GetASID();
		if (size > brks[as]) {
			errno = E_NoMem;
			return (-1);
		}
		SetASID(0);
		r = p = brks[as] - size;
		p |= as << 52;
		for(q = p; q & 0xFFFFFFFFFFFFFL < brks[as];)
			q = pt_free(q);
		brks[as] = r;
		// Mark the last page
		if (r > 0) {
			pbl = pebble[as];
			basepg = (pbl & 0x65535);
			r = ((r >> 14) & 0xffff) + basepg;
			while (LockMMUSemaphore()==0);
			pte = page_table[r];
			pte &= 0xfcffffff;
			pte |= 0x01000000;
			page_table[r] = pte;
			UnlockMMUSemaphore();
		}
		SetASID(as);
	}
	else {	// size==0
		as = GetASID();
		p = brks[as];	
	}
	return (p);
}
