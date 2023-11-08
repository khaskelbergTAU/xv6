// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct bufbucket
{
  struct spinlock lock;
  char lockname[20];
  struct buf *buf;
};

struct
{
  struct spinlock lock;
  struct buf buf[NBUF];

  struct bufbucket buckets[NBUCKETS];
} bcache;

void binit(void)
{
  struct buf *b;

  // init locks
  initlock(&bcache.lock, "bcache");
  for (uint bckt = 0; bckt < NBUCKETS; bckt++)
  {
    snprintf(bcache.buckets[bckt].lockname, 20, "bcache_%d", bckt);
    initlock(&bcache.buckets[bckt].lock, bcache.buckets[bckt].lockname);
    bcache.buckets[bckt].buf = 0;
  }

  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = 0;
    b->prev = 0;
    initsleeplock(&b->lock, "buffer");
  }
}

// unlink buf from the hash table
static void unlink(struct buf *buf)
{
  uint bckt = buf->blockno % NBUCKETS;
  if (bcache.buckets[bckt].buf == buf) // its the head
  {
    bcache.buckets[bckt].buf = buf->next;
  }
  if (buf->prev != 0)
  {
    buf->prev->next = buf->next;
  }
  if (buf->next != 0)
  {
    buf->next->prev = buf->prev;
  }
  buf->next = 0;
  buf->prev = 0;
}
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bckt = blockno % NBUCKETS;

  acquire(&bcache.buckets[bckt].lock);

  // Is the block already cached?
  for (b = bcache.buckets[bckt].buf; b != 0; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.buckets[bckt].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  release(&bcache.buckets[bckt].lock);
  // Not cached.
  // find an unused buffer
  acquire(&bcache.lock);
  acquire(&bcache.buckets[bckt].lock);

  // Check again, this time serialized: Is the block already cached?
  for (b = bcache.buckets[bckt].buf; b != 0; b = b->next)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buckets[bckt].lock);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  for (uint i = 0; i < NBUF; i++)
  {
    b = &bcache.buf[i];
    uint old_bckt = b->blockno % NBUCKETS;
    if (old_bckt != bckt)
    {
      acquire(&bcache.buckets[old_bckt].lock);
    }
    if(b->refcnt == 0) {
      if (old_bckt != bckt)
      { // do we need to move the block to another bucket?
        unlink(b);
        release(&bcache.buckets[old_bckt].lock);
        b->next = bcache.buckets[bckt].buf;
        if (b->next != 0)
        {
          b->next->prev = b;
        }
        bcache.buckets[bckt].buf = b;
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      release(&bcache.buckets[bckt].lock);
      acquiresleep(&b->lock);
      return b;
    }
    if (old_bckt != bckt)
    {
      release(&bcache.buckets[old_bckt].lock);
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint bckt = b->blockno % NBUCKETS;

  acquire(&bcache.buckets[bckt].lock);
  b->refcnt--;
  release(&bcache.buckets[bckt].lock);
}

void
bpin(struct buf *b) {
  uint bckt = b->blockno % NBUCKETS;
  acquire(&bcache.buckets[bckt].lock);
  b->refcnt++;
  release(&bcache.buckets[bckt].lock);
}

void
bunpin(struct buf *b) {
  uint bckt = b->blockno % NBUCKETS;
  acquire(&bcache.buckets[bckt].lock);
  b->refcnt--;
  release(&bcache.buckets[bckt].lock);
}


