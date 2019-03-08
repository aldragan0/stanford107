#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn,
                HashSetFreeFunction freefn)
{
  assert(elemSize > 0);
  assert(numBuckets > 0);
  assert(hashfn != NULL);
  assert(comparefn != NULL);

  h->numBuckets = numBuckets;
  h->logSize = 0;
  h->hashFn = hashfn;
  h->compareFn = comparefn;
  h->buckets = (vector*)malloc(h->numBuckets * sizeof(*(h->buckets)));

  for(int i = 0; i < h->numBuckets; ++i){
    VectorNew(&(h->buckets)[i], elemSize, freefn, 0);
  }
}

void HashSetDispose(hashset *h)
{
  for(int i = 0; i < h->numBuckets; ++i){
    VectorDispose(&(h->buckets)[i]);
  }
  free(h->buckets);
}

int HashSetCount(const hashset *h)
{
  return h->logSize;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
  assert(mapfn != NULL);
  for(int i = 0; i < h->numBuckets; ++i){
    VectorMap(&(h->buckets)[i], mapfn, auxData);
  }
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
  assert(elemAddr != NULL);
  int elemHashCode = h->hashFn(elemAddr, h->numBuckets);
  assert(elemHashCode >= 0 && elemHashCode < h->numBuckets);
  
  vector *elemBucket = &(h->buckets)[elemHashCode];
  int oldElemPosition = VectorSearch(elemBucket, elemAddr,
                                     h->compareFn, 0, false);
  if(oldElemPosition != -1){
    VectorReplace(elemBucket, elemAddr, oldElemPosition);
  }
  else{
    VectorAppend(elemBucket, elemAddr);
    ++h->logSize;
  }
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{
  assert(elemAddr != NULL);
  int elemHashCode = h->hashFn(elemAddr, h->numBuckets);
  assert(elemHashCode >= 0 && elemHashCode < h->numBuckets);

  vector *elemBucket = &(h->buckets)[elemHashCode];

  int elemPos = VectorSearch(elemBucket, elemAddr,
                             h->compareFn, 0, false);
  if(elemPos != -1){
    return VectorNth(elemBucket, elemPos);
  }
  return NULL;
}
