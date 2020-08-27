#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {

  struct proc *curproc = myproc();
  uint sz = PGROUNDUP(curproc->sz);
  int i;
  int match = 0;

  acquire(&(shm_table.lock));
  for(i = 0; i < 64; i++){
	if(shm_table.shm_pages[i].id == id){
		match = 1;
		mappages(curproc->pgdir, (char *)sz, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
		shm_table.shm_pages[i].refcnt++;
		break;
	}
  }  

  if(match == 0){
	for(i = 0; i < 64; i++){
		if(shm_table.shm_pages[i].id == 0){
			shm_table.shm_pages[i].id = id;
			break;	
		}
	}
	shm_table.shm_pages[i].frame = kalloc();
	memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
	mappages(curproc->pgdir, (char *)sz, PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
        shm_table.shm_pages[i].refcnt++;
  }
  
  curproc->sz = sz + PGSIZE;
  *pointer = (char *)sz;	
  release(&(shm_table.lock));
  return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {

  int i;

  acquire(&(shm_table.lock));
  for(i = 0; i < 64; ++i) {
	if(shm_table.shm_pages[i].id == id){
		if(shm_table.shm_pages[i].refcnt > 0){
			shm_table.shm_pages[i].refcnt--;
		} else {
			shm_table.shm_pages[i].id = 0;
			shm_table.shm_pages[i].frame = 0;
			break;
		}
	}
  }
  release(&(shm_table.lock));

  return 0; //added to remove compiler warning -- you should decide what to return
}
