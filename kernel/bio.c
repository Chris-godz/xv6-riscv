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

#define NBUCKET 13
#define HASH(bktnum) ((bktnum)%NBUCKET)

struct bucket
{
    struct spinlock lock;
    struct buf *next;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct bucket head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.head[i].lock, "bcache");
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->next = bcache.head[0].next;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b,*prev = 0;

  int id = HASH(blockno);
  acquire(&bcache.head[id].lock);
  b = bcache.head[id].next;

  while (b) {
    if (b->dev == dev && b->blockno == blockno) {
        b->refcnt++;
        release(&bcache.head[id].lock);
        acquiresleep(&b->lock);
        return b;
    }
    b = b->next;
  }
  release(&bcache.head[id].lock);

  // Not cached.
  int i = id;
  do {
    acquire(&bcache.head[i].lock);
    b = bcache.head[i].next;
    while (b) {
      if( b->refcnt == 0)
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        if(i != id){
          if(b != bcache.head[i].next){
            prev->next = b->next;
          }else {
            bcache.head[i].next = b->next;
          }
          release(&bcache.head[i].lock);
          acquire(&bcache.head[id].lock);
          b->next = bcache.head[id].next;
          bcache.head[id].next = b;
          release(&bcache.head[id].lock);
        }else {
          release(&bcache.head[i].lock);
        }
        acquiresleep(&b->lock);
        return b;
      }
      prev = b;
      b = b->next;
    }
    release(&bcache.head[i].lock);
    i = HASH(i+1);
  }while (i != id);

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
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int id = HASH(b->blockno);
  acquire(&bcache.head[id].lock);
  b->refcnt--;
  release(&bcache.head[id].lock);
}

void
bpin(struct buf *b) {
  int id = HASH(b->blockno);
  acquire(&bcache.head[id].lock);
  b->refcnt++;
  release(&bcache.head[id].lock);
}

void
bunpin(struct buf *b) {
  int id = HASH(b->blockno);
  acquire(&bcache.head[id].lock);
  b->refcnt--;
  release(&bcache.head[id].lock);
}


