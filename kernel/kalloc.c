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
  uint32 refcnt[MAXPAGES];
} kmem;

inline uint32 pa2index(uint64 pa)
{
  if(pa < (uint64)end)
    panic("pa2index");
  uint32 i = (pa - (uint64)end) / PGSIZE;
  if(i >= MAXPAGES)
    panic("pa2index");
  return i;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    kmem.refcnt[pa2index((uint64)p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint32 i = pa2index((uint64)pa);
  if(kmem.refcnt[i] == 0){
    panic("kfree : refcnt");
  }

  acquire(&kmem.lock);
  // Fill with junk to catch dangling refs.
  if( 0 == --kmem.refcnt[i])
  {
    memset(pa, 1, PGSIZE);
    r = (struct run*)pa;
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

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    kmem.refcnt[pa2index((uint64)r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

uint32 r_ref(void *pa){
  uint32 i = pa2index((uint64)pa);
  return kmem.refcnt[i];
}

void a_ref(void* pa){
  uint32 i = pa2index((uint64)pa);
  acquire(&kmem.lock);
  if(kmem.refcnt[i] == 0)
    panic("a_ref");
  kmem.refcnt[i]++;
  // if(idx<10000)
  //   printf("inc ref at pa %ld,idx is %d,refcount is %d\n ", (uint64)pa,idx, kmem.refcount[idx]);
  release(&kmem.lock);
}