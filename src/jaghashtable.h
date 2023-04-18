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
#ifndef _JAG_HASH_H_
#define _JAG_HASH_H_

typedef struct HashNodeT 
{
  char * key;  
  char * value;
  //bool   valueReadOnly;
  struct HashNodeT *next;   /* next node in hash chain */
} HashNodeT;

class jag_hash_t 
{
  public:
  jag_hash_t() { doneInit = false; }

  struct HashNodeT **bucket;        /* array of hash nodes */
  int size;                           /* size of the array */
  int entries;                        /* number of entries in table */
  int downshift;                      /* shift cound, used in hash function */
  int mask;                           /* used to select bits for hashing */
  unsigned char empty;
  bool doneInit;
};

void jag_hash_init(jag_hash_t *, int buckets);

// NULL is not found
char *jag_hash_lookup (const jag_hash_t *t, const char *key);

int jag_hash_insert (jag_hash_t *t, const char *key, const char *val );
int jag_hash_delete (jag_hash_t *t, const char * key, bool freeval=true );

int jag_hash_insert_str_voidptr(jag_hash_t *htabptr, const char *key, void *value );

int jag_hash_insert_int_int (jag_hash_t *t, int key, int val );
bool jag_hash_lookup_int_int (const jag_hash_t *t, int key, int *val );
int jag_hash_delete_int (jag_hash_t *t, int key );

int jag_hash_insert_str_int (jag_hash_t *t, const char *key, int val );
bool jag_hash_lookup_str_int (const jag_hash_t *t, const char *key, int *val );


void jag_hash_destroy(jag_hash_t *t, bool freeval=true);
void jag_hash_print(jag_hash_t *t);

#endif

