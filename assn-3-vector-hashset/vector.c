#include "vector.h"
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
 assert(elemSize > 0);
 assert(initialAllocation >= 0);

 v->elemSize = elemSize;
 v->freeFn = freeFn;
 v->allocSize = (initialAllocation) ? initialAllocation : 4;
 v->logSize = 0;
 v->elems = malloc(v->allocSize * v->elemSize);
}

void VectorDispose(vector *v)
{
  if(v->freeFn != NULL){
    for(size_t i = 0; i < v->logSize; ++i){
      void *elemAddr = (char*)v->elems + i * v->elemSize;
      v->freeFn(elemAddr);
    }
  }
  free(v->elems);
  memset(v, 0, sizeof(*v));
}

static void privateVectorEnlarge(vector* v)
{
  if(v->logSize < v->allocSize) return;
  v->allocSize += v->allocSize;
  v->elems = realloc(v->elems, v->elemSize * v->allocSize);
  assert(v->elems != NULL);
}

int VectorLength(const vector *v)
{ return (v->logSize); }

void *VectorNth(const vector *v, int position)
{ 
  assert((position >= 0) && (position < v->logSize));
  return (void*)((char*)v->elems + position * v->elemSize);
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
  assert((position >= 0) && (position < v->logSize));
  void* oldElemAddr = (char*)v->elems + position * v->elemSize;
  if(v->freeFn){
    v->freeFn(oldElemAddr);
  }
  memcpy(oldElemAddr, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
  assert((position >= 0) && (position <= v->logSize));

  privateVectorEnlarge(v);
  void *oldElemAddr = (char*)v->elems + position * v->elemSize;

  if(position == v->logSize){
    memcpy(oldElemAddr, elemAddr, v->elemSize);
  }
  void *newElemAddr = (char*)oldElemAddr + v->elemSize;
  size_t numElems = v->logSize - position;
  memmove(newElemAddr, oldElemAddr, numElems * v->elemSize);
  memcpy(oldElemAddr, elemAddr, v->elemSize);
  ++v->logSize;
}

void VectorAppend(vector *v, const void *elemAddr)
{
  privateVectorEnlarge(v);
  void *newElemAddr = (char*)v->elems + v->logSize * v->elemSize;
  memcpy(newElemAddr, elemAddr, v->elemSize);
  ++v->logSize;
}

void VectorDelete(vector *v, int position)
{
  assert((position >= 0) && (position < v->logSize));
  void *elemAddr = (char*)v->elems + position * v->elemSize;

  if(v->freeFn) v->freeFn(elemAddr);
  if(position != v->logSize - 1){
    void *oldElemAddr = (char*)elemAddr + v->elemSize;
    size_t numElems = v->logSize - position - 1;
    memcpy(elemAddr, oldElemAddr, numElems * v->elemSize);
  }
  --v->logSize;
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
  assert(compare != NULL);
  qsort(v->elems, v->logSize, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
  assert(mapFn != NULL);
  for(int i = 0; i < v->logSize; ++i){
    void *elemAddr = (char*)v->elems + i * v->elemSize;
    mapFn(elemAddr, auxData);
  }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{
  assert(searchFn != NULL);
  assert(key != NULL);
  if(startIndex >= v->logSize) return kNotFound;
  assert((startIndex >= 0) && (startIndex < v->logSize));
  void *baseElemAddr = (char*)v->elems + startIndex * v->elemSize;
  size_t numElems = v->logSize - startIndex;
  void *foundElemAddr;
  if(isSorted){
    foundElemAddr = bsearch(key, baseElemAddr, numElems, v->elemSize, searchFn);
  }
  else{
    foundElemAddr = lfind(key, baseElemAddr, &numElems, v->elemSize, searchFn);
  }
  if(!foundElemAddr) return kNotFound;
  int position = ((char*)foundElemAddr - (char*)v->elems) / v->elemSize;
  return position;
} 
