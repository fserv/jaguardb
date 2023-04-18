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
#ifndef _JAG_HASH_SET_H_
#define _JAG_HASH_SET_H_

typedef struct HashSetNodeT 
{
  char * key;  
  struct HashSetNodeT *next;
} HashSetNodeT;

typedef struct jag_hash_set_t 
{
  struct HashSetNodeT **bucket;       /* array of hash nodes */
  int size;                           /* size of the array */
  int entries;                        /* number of entries in table */
  int downshift;                      /* shift cound, used in hash function */
  int mask;                           /* used to select bits for hashing */
  unsigned char empty;
} jag_hash_set_t;

void jag_hash_set_init(jag_hash_set_t *, int buckets);

// false if not found; true if found
bool jag_hash_set_lookup (const jag_hash_set_t *t, const char *key);

int jag_hash_set_insert (jag_hash_set_t *t, const char *key );
int jag_hash_set_delete (jag_hash_set_t *t, const char * key );
void jag_hash_set_destroy(jag_hash_set_t *t );
void jag_hash_set_print(jag_hash_set_t *t);

#endif

