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
#include "jaghashset.h"

#define HASH_SET_LIMIT 0.5

static int sethashcode(const jag_hash_set_t *htabptr, const char *key) {
  int i=0;
  int hashvalue;
  while (*key != '\0') i=(i<<3)+(*key++ - '0');
  hashvalue = (((i*1103515249)>>htabptr->downshift) & htabptr->mask);
  if (hashvalue < 0) {
    hashvalue = 0;
  }    
  return hashvalue;
}

void jag_hash_set_init(jag_hash_set_t *htabptr, int buckets) {

  if (buckets==0) buckets=143;

  htabptr->entries=0;
  htabptr->size=2;
  htabptr->mask=1;
  htabptr->downshift=29;

  while (htabptr->size < buckets) {
    htabptr->size<<=1;
    htabptr->mask=(htabptr->mask<<1)+1;
    htabptr->downshift--;
  }

  htabptr->bucket=(HashSetNodeT **) calloc(htabptr->size, sizeof(HashSetNodeT *));
  htabptr->empty = 0;
  return;
}

static void rebuild_table(jag_hash_set_t *htabptr) 
{
  HashSetNodeT **old_bucket, *old_hash, *tmp;
  int old_size, h, i;

  old_bucket=htabptr->bucket;
  old_size=htabptr->size;

  jag_hash_set_init(htabptr, old_size<<1);

  for (i=0; i<old_size; i++) {
    old_hash=old_bucket[i];

    while(old_hash) {
      tmp=old_hash;
      old_hash=old_hash->next;
      h=sethashcode(htabptr, tmp->key);
      tmp->next=htabptr->bucket[h];
      htabptr->bucket[h]=tmp;
      htabptr->entries++;
    }
  }

  if ( old_bucket ) free(old_bucket);
  old_bucket = NULL;
  return;
}

bool jag_hash_set_lookup(const jag_hash_set_t *htabptr, const char *key )
{
  int h;
  HashSetNodeT *node;

  h=sethashcode(htabptr, key);
  for (node=htabptr->bucket[h]; node!=NULL; node=node->next) {
    if (!strcmp(node->key, key))
      break;
  }

  return(node ? true : false );
}

int jag_hash_set_insert(jag_hash_set_t *htabptr, const char *key ) 
{
  HashSetNodeT *node;
  int h;

  if ( jag_hash_set_lookup(htabptr, key) ) {
      return 0; 
  }

  while (htabptr->entries>=HASH_SET_LIMIT*htabptr->size)
    rebuild_table(htabptr);

  h=sethashcode(htabptr, key);
  node=(struct HashSetNodeT *) malloc(sizeof(HashSetNodeT));
  node->key= strdup(key);
  node->next=htabptr->bucket[h];
  htabptr->bucket[h]=node;
  htabptr->entries++;

  return 1;
}

int jag_hash_set_delete(jag_hash_set_t *htabptr, const char *key ) 
{
  HashSetNodeT *node, *last;
  int h;

  h=sethashcode(htabptr, key);
  for (node=htabptr->bucket[h]; node; node=node->next) {
    if (!strcmp(node->key, key))
      break;
  }

  if (node==NULL) return 0;

  if (node==htabptr->bucket[h])
    htabptr->bucket[h]=node->next;
  else {
    for (last=htabptr->bucket[h]; last && last->next; last=last->next) {
      if (last->next==node) break;
    }
    last->next=node->next;
  }

  if ( node->key ) free( node->key );
  if ( node ) free(node);
  node = NULL;
  return 1; 
}

void jag_hash_set_destroy(jag_hash_set_t *htabptr ) 
{
  HashSetNodeT *node, *last;
  int i;

  for (i=0; i<htabptr->size; i++) {
    node = htabptr->bucket[i];
    while (node != NULL) { 
      last = node;   
      node = node->next;
	  if ( last->key ) free(last->key);
      if ( last ) free(last);
	  last = NULL;
    }
  }     

  if (htabptr->bucket != NULL) {
    free(htabptr->bucket);
    memset(htabptr, 0, sizeof(jag_hash_set_t));
  }

  htabptr->empty = 1;

}

void jag_hash_set_print(jag_hash_set_t *htabptr) 
{
  HashSetNodeT *node;
  int i;

  for (i=0; i<htabptr->size; i++) {
    node = htabptr->bucket[i];
    while (node != NULL) { 
	  printf("bucket=%d key=[%s]\n", i, node->key );
      node = node->next;
    }
  }     

}


