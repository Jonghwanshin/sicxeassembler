#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <stdlib.h>
#include <limits.h>
#include <string.h>

struct unresolved_node_s {
	unsigned int address;
	struct unresolved_node_s *next;
};

typedef struct unresolved_node_s unresolved_node;

typedef struct entry_s {
	char *key;
	unsigned int value;
	unresolved_node *unresolved;
	struct entry_s *next;
} entry_t;

//typedef  entry_s entry_t;

struct hashtable_s {
	int size;
	struct entry_s **table;	
};

typedef struct hashtable_s hashtable_t;

hashtable_t *ht_create( int size );
int ht_hash( hashtable_t *hashtable, char *key );
entry_t *ht_newpair( char *key, unsigned int value );
void ht_set( hashtable_t *hashtable, char *key, unsigned int value, unsigned int currentaddress );
entry_t* ht_get( hashtable_t *hashtable, char *key );

unsigned int ShowFirstNode(unresolved_node* header);
unresolved_node* NewUnresolvedNode(unresolved_node* header, unsigned int address);

#endif