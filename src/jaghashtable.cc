/*
 * Copyright JaguarDB
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "jaghashtable.h"

#define HASH_LIMIT 0.5

static int hashcode(const jag_hash_t *htabptr, const char *key) {
  int i=0;
  int hashvalue;
 
  while (*key != '\0')
    i=(i<<3)+(*key++ - '0');
 
  hashvalue = (((i*1103515249)>>htabptr->downshift) & htabptr->mask);
  if (hashvalue < 0) {
    hashvalue = 0;
  }    

  return hashvalue;
}

/*
 *  Initialize a new hash table.
 *  htabptr: Pointer to the hash table to initialize
 *  buckets: The number of initial buckets to create
 */
void jag_hash_init(jag_hash_t *htabptr, int buckets) {

  /* make sure we allocate something */
  if (buckets==0)
    buckets=143;

  /* initialize the table */
  htabptr->entries=0;
  htabptr->size=2;
  htabptr->mask=1;
  htabptr->downshift=29;

  /* ensure buckets is a power of 2 */
  while (htabptr->size < buckets) {
    htabptr->size<<=1;
    htabptr->mask=(htabptr->mask<<1)+1;
    htabptr->downshift--;
  } /* while */

  /* allocate memory for table */
  htabptr->bucket=(HashNodeT **) calloc(htabptr->size, sizeof(HashNodeT *));

  htabptr->empty = 0;
  htabptr->doneInit = true;

  return;
}

/*
 *  Create new hash table when old one fills up.
 *  htabptr: Pointer to a hash table
 */
static void rebuild_table(jag_hash_t *htabptr) 
{
  HashNodeT **old_bucket, *old_hash, *tmp;
  int old_size, h, i;

  old_bucket=htabptr->bucket;
  old_size=htabptr->size;

  /* create a new table and rehash old buckets */
  jag_hash_init(htabptr, old_size<<1);

  for (i=0; i<old_size; i++) {
    old_hash=old_bucket[i];

    while(old_hash) {
      tmp=old_hash;
      old_hash=old_hash->next;
      h=hashcode(htabptr, tmp->key);
      tmp->next=htabptr->bucket[h];
      htabptr->bucket[h]=tmp;
      htabptr->entries++;
    }

  }

  /* free memory used by old table */
  if ( old_bucket ) free(old_bucket);
  old_bucket = NULL;

  return;
}

/*
 *  Lookup an entry in the hash table and return a pointer to
 *  it or NULL if it wasn't found.
 *  htabptr: Pointer to the hash table
 *  key: The key to lookup
 *  returns: pointer to value (no memory allocated, just use it)
 */
char * jag_hash_lookup(const jag_hash_t *htabptr, const char *key )
{
  int h;
  HashNodeT *node;

  /* find the entry in the hash table */
  h=hashcode(htabptr, key);
  for (node=htabptr->bucket[h]; node!=NULL; node=node->next) {
    if (node->key && !strcmp(node->key, key))
      break;
  }

  /* return the entry if it exists, or NULL */
  return(node ? node->value : NULL);
}

/*
 *  Insert an entry into the hash table.  If the entry already
 *  exists return a pointer to it, otherwise return -1.
 *
 *  htabptr: A pointer to the hash table
 *  key: The key to insert into the hash table
 *  data: A pointer to the data to insert into the hash table
 *
 *  key and value are memory allocated in the hash (strdup)
 */
int jag_hash_insert(jag_hash_t *htabptr, const char *key, const char *value ) 
{
  HashNodeT *node;
  int h;

  if ( NULL != jag_hash_lookup(htabptr, key) ) {
      return 0; 
  }

  while (htabptr->entries>=HASH_LIMIT*htabptr->size)
    rebuild_table(htabptr);

  h=hashcode(htabptr, key);
  node=(struct HashNodeT *) malloc(sizeof(HashNodeT));
  node->key= strdup(key);
  node->value= strdup(value); 
  node->next=htabptr->bucket[h];
  htabptr->bucket[h]=node;
  htabptr->entries++;

  return 1;
}

int jag_hash_insert_str_voidptr(jag_hash_t *htabptr, const char *key, void *value ) 
{
  HashNodeT *node;
  int h;

  if ( NULL != jag_hash_lookup(htabptr, key) ) {
      return 0; 
  }

  while (htabptr->entries>=HASH_LIMIT*htabptr->size)
    rebuild_table(htabptr);

  h=hashcode(htabptr, key);
  node=(struct HashNodeT *) malloc(sizeof(HashNodeT));
  node->key= strdup(key);
  node->value= (char*)value; 
  node->next=htabptr->bucket[h];
  htabptr->bucket[h]=node;
  htabptr->entries++;

  return 1;
}

int jag_hash_delete_int(jag_hash_t *htabptr, int key ) 
{
	char sk[16];
	sprintf(sk, "%d", key );
	return jag_hash_delete( htabptr, sk, true );
}

/*
 *  Remove an entry from a hash table and return a pointer
 *  to its data or JAG_HASH_FAIL if it wasn't found.
 *
 *  htabptr: A pointer to the hash table
 *  key: The key to remove from the hash table
 */
int jag_hash_delete(jag_hash_t *htabptr, const char *key, bool freeval ) 
{
  HashNodeT *node, *last;
  int h;

  /* find the node to remove */
  h=hashcode(htabptr, key);
  for (node=htabptr->bucket[h]; node; node=node->next) {
    if ( node->key && !strcmp(node->key, key))
      break;
  }

  /* Didn't find anything, return JAG_HASH_FAIL */
  if (node==NULL)
    return 0;

  /* if node is at head of bucket, we have it easy */
  if (node==htabptr->bucket[h])
    htabptr->bucket[h]=node->next;
  else {
    /* find the node before the node we want to remove */
    for (last=htabptr->bucket[h]; last && last->next; last=last->next) {
      if (last->next==node) break;
    }
    last->next=node->next;
  }

  if ( node->key ) free( node->key );
  if ( freeval && node->value ) free( node->value );
  if ( node ) free(node);
  node = NULL;

  return 1; 
}

void jag_hash_destroy(jag_hash_t *htabptr, bool freeval ) 
{
  if ( htabptr->doneInit == false ) {
  	  return;
  }

  HashNodeT *node, *last;
  int i;

  for (i=0; i<htabptr->size; i++) {
    node = htabptr->bucket[i];
    while (node != NULL) { 
      last = node;   
      node = node->next;
	  if ( last->key ) free(last->key);
      if ( freeval && last->value ) free(last->value);
      if ( last ) free(last);
	  last = NULL;
    }
  }     

  /* free the entire array of buckets */
  if (htabptr->bucket != NULL && htabptr->size > 0) {
    free(htabptr->bucket);
    htabptr->bucket = NULL;
    memset(htabptr, 0, sizeof(jag_hash_t));
  }

  htabptr->empty = 1;
  htabptr->entries = 0;
  htabptr->doneInit = false;

}

void jag_hash_print(jag_hash_t *htabptr) 
{
  HashNodeT *node;
  int i;

  for (i=0; i<htabptr->size; i++) {
    node = htabptr->bucket[i];
    while (node != NULL) { 
	  printf("bucket=%d key=[%s]  value=[%s]\n", i, node->key, node->value );
      node = node->next;
    }
  }     

}


int jag_hash_insert_int_int (jag_hash_t *t, int key, int val )
{
	char sk[16], sv[16];
	sprintf(sk, "%d", key );
	sprintf(sv, "%d", val );
	return jag_hash_insert( t, sk, sv );
}

bool jag_hash_lookup_int_int (const jag_hash_t *t, int key, int *val )
{
	char sk[16];
	sprintf(sk, "%d", key );
	char *pval = jag_hash_lookup( t, sk );
	if ( ! pval ) return false;
	*val = atoi( pval );
	return true;
}


int jag_hash_insert_str_int (jag_hash_t *t, const char *key, int val )
{
	char sv[16];
	sprintf(sv, "%d", val );
	return jag_hash_insert( t, key, sv );
}

bool jag_hash_lookup_str_int (const jag_hash_t *t, const char *key, int *val )
{
	char *pval = jag_hash_lookup( t, key );
	if ( ! pval ) return false;
	*val = atoi( pval );
	return true;
}


