#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Alphasize 26  // the no of children from a node can have a max of all alphabet letters   
#define Max 10       //Max no of suggestions 

typedef struct TrieNode {
    struct TrieNode *children[Alphasize];
    int Endofword;               // to mark the end of the word while traversing
}TrieNode;

TrieNode *createNode(){
    TrieNode *newnode = (TrieNode *)malloc(sizeof(struct TrieNode));    
    newnode->Endofword =0;
    for(int i =0 ; i < Alphasize; i++){
        newnode->children[i]=NULL;
    }
    return newnode;
}

void insertword(TrieNode *root , const char *word){
    TrieNode *curr = root;
    while(*word){
        int index = *word  - 'a';         //for eg: a - 'a' gives 0 
        if(!curr->children[index]){
            curr->children[index]=createNode();     
        }
        curr = curr->children[index];  //traversing
        word++;
    }
    curr->Endofword =1;
}

void findsuggestions(TrieNode *root, char *prefix ,int level, int *count){
   if(*count >=Max){
    return;
   }
   
    if(root->Endofword){    //if that is the last character of the word
        printf("%s\n",prefix);
        (*count)++;
    }

    for(int i=0 ; i < Alphasize ; i++){
        if(root->children[i]){  //if it has children or next letter
            prefix[level] = 'a' + i ;  //that next letter is added to the prefix and null character is added at end
            prefix[level + 1] = '\0';
            findsuggestions(root->children[i],prefix,level+1, count); 
        }
    }
}

void autocomplete(TrieNode *root, const char *prefix , int *count){
    TrieNode *curr =root;
    while(*prefix){     //until all the previous characters are traversed
        int index = *prefix - 'a';  
        if (!curr->children[index]){  
            printf("No suggestions found!\n");    //since no child even though prefix is present
            return;
        }
        curr = curr->children[index];
        prefix++;
    }
    char buffer[50];
    strcpy(buffer, prefix);
    findsuggestions(curr, buffer, strlen(buffer), count);  // find all the suggestions
}

void loadDictionary(TrieNode *root, const char *filename) {
    FILE *file = fopen(filename, "r");                          // name of the fie is stored in a variable 
    if (!file) {
        printf("Could not open file %s\n", filename);
        return;
    }
    char word[50];
    while (fscanf(file, "%s", word) != EOF) {
        insertword(root, word);        //inserting all the words of the file into trie tree
    }
    fclose(file);
}

int main() {
    TrieNode *root = createNode();
    
    loadDictionary(root, "dictionary.txt");// Load words from dictionary file

    char query[20];
    printf("Enter a prefix: ");
    scanf("%s", query);
    
    printf("Autocomplete Suggestions:\n");
    int count=0; 
    autocomplete(root, query, &count);
    return 0;
}