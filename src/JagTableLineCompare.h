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
#include <abax.h>
#include <vector>
#include <JagRecord.h>
#include <JagSchemaRecord.h>

#define BUFF_LEN 8
#define COMP_LEN 2

class JagTableLineCompare 
{
	public:
		JagTableLineCompare(const std::vector<Jstr>& sortBy, const JagSchemaRecord& raySchema) {
			sort_vec = sortBy;
			schema = raySchema;
			reverse = false;
		}

		JagTableLineCompare(const std::vector<Jstr>& sortBy, const JagSchemaRecord& raySchema, bool flag) {
			sort_vec = sortBy;
			schema = raySchema;
			reverse = flag;
		}


		bool operator() (const JagTableLine& a, const JagTableLine& b) {
			Jstr c1;
			Jstr c2;
			char* ptr;
			for(int i = 0; i < sort_vec.size(); i++) {
				// Find out what the option is
				int isKey = JagTableLine::getKeyCol(JagTableLineCompare::sort_vec[i], schema);
				// Case 1: It is a key
				if(isKey != -1) {
					c1 = JagTableLine::getKeyString(a.line, isKey, schema);
					c2 = JagTableLine::getKeyString(b.line, isKey, schema);
				}
				// Case 2: It is a value
				else {
					JagRecord* aRecord = JagTableLine::createJagRecord(schema, a.line);
					JagRecord* bRecord = JagTableLine::createJagRecord(schema, b.line);
					ptr = aRecord->getValue((sort_vec[i]).c_str());
					if(ptr != NULL) {
						c1 = Jstr(ptr);
						if ( ptr ) free(ptr);
						ptr = NULL;
						if ( aRecord ) delete(aRecord);
						if ( bRecord ) delete(bRecord);
						aRecord = bRecord = NULL;
						break;
					}

					ptr = bRecord->getValue((sort_vec[i]).c_str());
					if(ptr != NULL) {
						c2 = Jstr(ptr);
						if ( ptr ) free(ptr);
						ptr = NULL;
						if ( aRecord ) delete(aRecord);
						if ( bRecord ) delete(bRecord);
						aRecord = bRecord = NULL;
						break;
					}

					if ( aRecord ) delete(aRecord);
					if ( bRecord ) delete(bRecord);
					aRecord = bRecord = NULL;
					//}
				}

				int result = c1.compare(c2);
				if(!reverse) {
					if(result > 0) {
						return false;
					} else if(result < 0) {
						return true;
					}
				} else {
					if(result > 0) {
						return true;
					} else if(result < 0) {
						return false;
					}
				}
			}

			return true;
		}

		std::vector<Jstr> sort_vec;
		JagSchemaRecord schema;
		bool reverse;
};
