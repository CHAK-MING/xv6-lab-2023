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

struct {
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct BucketType {
    struct spinlock lock;
    struct buf head;
  } buckets[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;
  int i;
  for(i = 0; i < NBUCKETS; ++i) {
    char name_buf[16] = {0};
    snprintf(name_buf, 16, "bcache_lock_%d", i);
    initlock(&bcache.buckets[i].lock, name_buf);
    bcache.buckets[i].head.next = 0;
  }

  for(i = 0; i < NBUF; ++i)
    bcache.buf[i].timestamp = ticks;

  // 挂在0号哈希桶的，用头插法
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].head.next;
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = (dev + blockno) % NBUCKETS;
  struct BucketType* bucket = &bcache.buckets[id];
  acquire(&bucket->lock);

  // Is the block already cached?
  for(b = bucket->head.next; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 再占用一个锁的情况下，如果不先释放，可能会出现死锁的问题
  release(&bucket->lock);

  struct buf *before_latest = 0; // 要替换的bucket
  uint timestamp = 0;
  struct BucketType* max_bucket = 0;
 
  for(int i = 0; i < NBUCKETS; ++i) {
    int find_better = 0;
    struct BucketType* bucket = &bcache.buckets[i];
    acquire(&bucket->lock);
    for (b = &bucket->head; b->next; b = b->next) {
      if(b->next->refcnt == 0 && b->next->timestamp >= timestamp) {
        before_latest = b;
        timestamp = b->next->timestamp;
        find_better = 1;
      }
    }

    // 然后这个bucket有更大的timestamp，我们就使用
    if (find_better) {
      if(max_bucket != 0) release(&max_bucket->lock);
      max_bucket = bucket;
    } else {
      release(&max_bucket->lock);
    }
  }

  // 如果我们找到了一个buf，我们就先从这个桶删除这个buf
  // 并释放锁
  struct buf * res = before_latest->next;
  if (res != 0) {
    before_latest->next = before_latest->next->next;
    release(&max_bucket->lock);
  }
  // 现在我们将拿到的buf，插入到需要dev的bucket中
  acquire(&bucket->lock);
  if (res != 0) {
    res->next = bucket->head.next;
    bucket->head.next = res;
  }
  // 如果有另一个线程先进入了eviction，并且也正好是这个dev和blockno，
  // 我们再检查一遍，确保不会让一个dev对应一个buf
  for (b = bucket->head.next; b ; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  if (res == 0) panic("bget: no buffers");
  // 如果上面保证了还是找不到，那么就说明不会有重复，我们直接返回这个 buf（res)
  res->valid = 0;
  res->refcnt = 1;
  res->dev = dev;
  res->blockno = blockno;
  release(&bucket->lock);
  acquiresleep(&res->lock);
  return res;

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
  int id = (b->dev + b->blockno) % NBUCKETS;
  acquire(&bcache.buckets[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->timestamp = ticks;
  }
  release(&bcache.buckets[id].lock);
}

void
bpin(struct buf *b) {
  int id = (b->dev + b->blockno) % NBUCKETS;
  acquire(&bcache.buckets[id].lock);
  b->refcnt++;
  release(&bcache.buckets[id].lock);
}

void
bunpin(struct buf *b) {
  int id = (b->dev + b->blockno) % NBUCKETS;
  acquire(&bcache.buckets[id].lock);
  b->refcnt--;
  release(&bcache.buckets[id].lock);
}


