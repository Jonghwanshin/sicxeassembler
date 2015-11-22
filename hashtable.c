#define _XOPEN_SOURCE 500 /* Enable certain library functions (strdup) on linux.  See feature_test_macros(7) */

#include "hashtable.h"

hashtable_t* SYMTAB;	//SYMTAB DEFINITION
//hashtable_t* LITTAB;	//LITTAB Definition
unresolved_node* MDRTAB;	//Modification Record Table Definition

/* Create a new hashtable. */
hashtable_t *ht_create( int size ) {

	hashtable_t *hashtable = NULL;
	int i;

	if( size < 1 ) return NULL;

	/* Allocate the table itself. */
	hashtable = (hashtable_t *)malloc( sizeof( hashtable_t ));
	if( hashtable == NULL ) {
		return NULL;
	}

	/* Allocate pointers to the head nodes. */
	hashtable->table = (entry_t **)malloc(sizeof(entry_t *) * size);
	if( hashtable->table == NULL ){
		return NULL;
	}
	for(i = 0; i < size; i++ )
	{
		hashtable->table[i] = NULL;
	}

	hashtable->size = size;

	return hashtable;	
}

/* Hash a string for a particular hash table. */
int ht_hash( hashtable_t *hashtable, char *key ) {

	unsigned long int hashval = 0;
	int i = 0;

	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
	}

	return hashval % hashtable->size;
}

/* Create a key-value pair. */
entry_t *ht_newpair( char *key, unsigned int value ) {
	entry_t *newpair;

	newpair = (entry_t *)malloc(sizeof(entry_t));
	newpair->key = strdup(key);
	newpair->value = value;
	newpair->unresolved = NULL;
	newpair->next = NULL;

	if( newpair == NULL ) {
		return NULL;
	}

	if( newpair->key == NULL ) {
		return NULL;
	}



	return newpair;
}

/* Insert a key-value pair into a hash table. */
void ht_set( hashtable_t *hashtable, char *key, unsigned int value, unsigned int currentaddress) {
	int bin = 0;
	entry_t *newpair = NULL;
	entry_t *next = NULL;
	entry_t *last = NULL;
	unresolved_node *u_node;
	bin = ht_hash( hashtable, key );

	next = hashtable->table[ bin ];

	while( next != NULL && next->key != NULL && strcmp( key, next->key ) > 0 ) {
		last = next;
		next = next->next;
	}

	if(value == 0){
		//If value is zero, need unresolved node for future use.
		u_node = (unresolved_node*)malloc(sizeof(unresolved_node));
		u_node->address = currentaddress;
		u_node->next = NULL;
	}

	/* There's already a pair. Change the value only if the input value is zero. */
	if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {
		//free( next->value );
		if(next->value == 0){
			if(value != 0){
				next->value = value;
			}
			else{
				//If Value is zero, add unresolved node to this pair with currentaddress.
				unresolved_node *next_u_node;
				next_u_node = next->unresolved;
				while(next_u_node->next != NULL){
					next_u_node = next_u_node->next;
				}
				next_u_node->next = u_node;
				//next->unresolved = u_node;
			}
		}
	/* Nope, could't find it.  Time to grow a pair. */
	} else {
		newpair = ht_newpair( key, value );
		if(value == 0){
			newpair->unresolved = u_node;
		}

		/* We're at the start of the linked list in this bin. */
		if( next == hashtable->table[ bin ] ) {
			newpair->next = next;
			hashtable->table[ bin ] = newpair;
	
		/* We're at the end of the linked list in this bin. */
		} else if ( next == NULL ) {
			last->next = newpair;
	
		/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
}

void ht_print(hashtable_t *hashtable, char* tablename){
	int n = 0;
	entry_t* header = hashtable->table[0];
	printf("---------TABLE: %s--------\n",tablename);
	printf("NAME           VALUE       \n");
	for(n = 0; n <hashtable->size ; n++){
		header = hashtable->table[n];
		if(header != NULL){
			if(*(header->key) != NULL){
				printf("%-10s     %06X\n",header->key,header->value);
			}
			header = header->next;
			while(header != NULL){
				printf("%-10s     %06X\n",header->key,header->value);
				header = header->next;
			}
		}
	}
}


/* Retrieve a key-value pair from a hash table. */
entry_t* ht_get( hashtable_t *hashtable, char *key ) {
	int bin = 0;
	entry_t *pair;

	bin = ht_hash( hashtable, key );

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) > 0 ) {
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
		return NULL;

	} else {
		return pair;
	}
	
}

/* Create a new unresolvedtable. */

unsigned int ShowFirstNode(unresolved_node* header){
	if(header != NULL){
		unresolved_node* temp = header;
		unsigned int value = temp->address;
		header = header->next;
		//free(temp);
		return value;
	}
}

unresolved_node* NewUnresolvedNode(unresolved_node* header, unsigned int address){
	unresolved_node* newnode = (unresolved_node *)malloc(sizeof(unresolved_node));
	unresolved_node* search;
	newnode->address = address;
	newnode->next = NULL;
	if(header == NULL){
		header = newnode;
		header->next = NULL;
	}else{
		search = header;
		while(search->next != NULL){
			search = search->next;
		}
		search->next = newnode;
	}
	return header;
}