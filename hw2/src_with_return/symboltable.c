#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	256

symtab * hash_table[TABLE_SIZE];
extern int linenumber;

int totalID=0;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}


void insertID(char *name){
	totalID++;
	int hash_key;
	symtab* ptr;
	symtab* symptr=(symtab*)malloc(sizeof(symtab));

	hash_key=HASH(name);
	ptr=hash_table[hash_key];

	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		// if( strcmp( ptr->lexeme, symptr->lexeme) < 0 ) {
			symptr->front=ptr;
			ptr->back=symptr;
			symptr->back=symptr;
			hash_table[hash_key]=symptr;
		// }
		// else {
		// 	while( strcmp( ptr->lexeme, symptr->lexeme) > 0){
		// 		ptr = ptr->back;
		// 	}
	 	// 	symptr->front=ptr;
		// 	ptr->back=symptr;
		// 	symptr->back=symptr;
		// }
	}

	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;
}

void printSym(symtab* ptr)
{
	    printf(" Name = %s \n", ptr->lexeme);
	    printf(" References = %d \n", ptr->counter);
}

int compare_nodeptr(const void* left, const void* right)
{
		return strcmp( (*(symtab**)left)->lexeme, (*(symtab**)right)->lexeme);
}

void printSymTab()
{
    int i;
/*
    printf("----- Symbol Table ---------\n");
    for (i=0; i<TABLE_SIZE; i++)
    {
        symtab* symptr;
	symptr = hash_table[i];
	while (symptr != NULL)
	{
      printf("====>  index = %d \n", i);
	    printSym(symptr);
	    symptr=symptr->front;
	}
    }
*/

		printf("Frequency of identifiers: \n");
		symtab ** nodeList = malloc( totalID * sizeof(symtab *)); // 300 is random;
		int idx = 0;
		for( i=0; i<TABLE_SIZE; i++){
			symtab* p;
			p = hash_table[i];
			while(p != NULL) {
				// printf("%s\t%d\n", p->lexeme,  p->counter);
				nodeList[idx++] = p;
				p = p->front;
			}
		}
		qsort( nodeList, totalID, sizeof(symtab *), compare_nodeptr);

		for( i=0; i<totalID; ++i)
			printf("%s\t%d\n", nodeList[i]->lexeme, nodeList[i]->counter);

		free(nodeList);
}
