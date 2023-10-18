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
} kmem;

struct {
  unsigned int table[PAGECOUNT];
  struct spinlock lock;
} page_refs;

void
actual_kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void inc_ref(void *pa)
{
  acquire(&page_refs.lock);
  page_refs.table[((uint64)pa - KERNBASE) >> 12]++;
  release(&page_refs.lock);
}

void dec_ref(void *pa)
{
  acquire(&page_refs.lock);
  unsigned int refc = page_refs.table[((uint64)pa - KERNBASE) >> 12]--; // post dec
  release(&page_refs.lock);
  if (refc == 1)
    actual_kfree(pa);
  if (refc == 0)
  {
    printf("dec_ref without inc_ref %p\n", pa);
    panic("dec_ref");
  }
}

void initrange(void *pa_start, void *pa_end)
{
  struct run *r;
  char *pa;
  pa = (char *)PGROUNDUP((uint64)pa_start);
  acquire(&kmem.lock);
  for (; pa + PGSIZE <= (char *)pa_end; pa += PGSIZE)
  {
    memset(pa, 1, PGSIZE);
    r = (struct run *)pa;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  release(&kmem.lock);
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initrange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    dec_ref(p);
}

// Free the page of physical memory pointed at by pa,
// which should have been returned by a call to kalloc(). 
void
kfree(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  dec_ref(pa);
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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    inc_ref(r);
  }
  return (void*)r;
}
