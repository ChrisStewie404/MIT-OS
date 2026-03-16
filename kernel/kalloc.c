// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int ref_cnts [PA2IDX(PHYSTOP)];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  int end_idx = PA2IDX(PGROUNDUP((uint64)end)), top_idx = PA2IDX(PHYSTOP);
  for(int i = 0; i < end_idx; i++)
    kmem.ref_cnts[i] = 0;
  for(int i = end_idx; i <= top_idx; i++)
    kmem.ref_cnts[i] = 1;
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int idx;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");


  r = (struct run*)pa;

  acquire(&kmem.lock);
  idx = PA2IDX(pa);
  if(kmem.ref_cnts[idx] == 0)
    panic("kfree: re-free page");
  kmem.ref_cnts[idx]--;

  if(kmem.ref_cnts[idx] == 0){
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int idx;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    idx = PA2IDX(r);
    if(kmem.ref_cnts[idx] != 0)
      panic("kalloc: referenced page");
    kmem.ref_cnts[idx]++;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
kref(void *pa)
{
  int idx;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kref");

  acquire(&kmem.lock);
  idx = PA2IDX(pa);
  kmem.ref_cnts[idx]++;

  release(&kmem.lock);
}

int
ksplit(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;
  int refs, idx;
  uint flags;
  char *mem;

  if((pte = walk(pagetable, va, 0)) == 0)
    return -1;
  pa = PTE2PA(*pte);
  flags = PTE_FLAGS(*pte);
  if(!(flags & PTE_COW) || (flags & PTE_W))
    return -1;

  idx = PA2IDX(pa);

  acquire(&kmem.lock);
  refs = kmem.ref_cnts[idx];
  if (refs == 0){
    goto err;
  } else if (refs == 1){
    *pte |= PTE_W;
    *pte &= (~PTE_COW);
  } else{
    release(&kmem.lock);
    mem = kalloc();
    acquire(&kmem.lock);
    if(mem == 0)
      goto err;
    memmove(mem, (void *)pa, PGSIZE);
    release(&kmem.lock);
    kfree((void*)pa);
    acquire(&kmem.lock);
    *pte = (PA2PTE(mem) | flags | PTE_W) & (~PTE_COW);
  }
  release(&kmem.lock);
  return 0;

  err:
    release(&kmem.lock);
    return -1;
}
