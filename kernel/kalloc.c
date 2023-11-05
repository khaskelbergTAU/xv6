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
  struct spinlock locks[NCPU];
  struct run *freelists[NCPU];
} kmem;

void
kinit()
{
  for (unsigned int cpu = 0; cpu < NCPU; cpu++)
  {
    char kmem_lock_name[20];
    snprintf(kmem_lock_name, 20, "kmem_%d", cpu);
    initlock(&kmem.locks[cpu], kmem_lock_name);
  }
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();
  pop_off();
  acquire(&kmem.locks[id]);
  r->next = kmem.freelists[id];
  kmem.freelists[id] = r;
  release(&kmem.locks[id]);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id = cpuid();
  pop_off();
  acquire(&kmem.locks[id]);
  r = kmem.freelists[id];
  if (r)
    kmem.freelists[id] = r->next;
  release(&kmem.locks[id]);
  if (!r)
  { // couldnt get a page from this cpus list
    for (unsigned int off = 1; off < NCPU; off++)
    { // start searching from the next cpu up to the previous one
      int cpu = (id + off) % NCPU;
      acquire(&kmem.locks[cpu]);
      r = kmem.freelists[cpu];
      if (r)
        kmem.freelists[cpu] = r->next;
      release(&kmem.locks[cpu]);
      if (r)
        break;
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
