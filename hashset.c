#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HASHSET_SIZE 100

typedef struct Node {
    int data;
    struct Node* next;
} Node;

typedef struct HashSet {
    Node* buckets[HASHSET_SIZE];
} HashSet;

// Hash function
int hash(int key) {
    return key % HASHSET_SIZE;
}

// Initialize a new HashSet
HashSet* initializeHashSet() {
    HashSet* set = (HashSet*)malloc(sizeof(HashSet));
    for (int i = 0; i < HASHSET_SIZE; i++) {
        set->buckets[i] = NULL;
    }
    return set;
}

// Insert a value into the HashSet
void insert(HashSet* set, int value) {
    int index = hash(value);
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = value;
    newNode->next = NULL;

    if (set->buckets[index] == NULL) {
        set->buckets[index] = newNode;
    } else {
        Node* current = set->buckets[index];
        while (current->next != NULL) {
            if (current->data == value) {
                return; // Value already exists, do not insert again
            }
            current = current->next;
        }
        current->next = newNode;
    }
}

// Check if a value exists in the HashSet
bool contains(HashSet* set, int value) {
    int index = hash(value);
    Node* current = set->buckets[index];

    while (current != NULL) {
        if (current->data == value) {
            return true; // Value found
        }
        current = current->next;
    }

    return false; // Value not found
}

// Remove a value from the HashSet
void removeValue(HashSet* set, int value) {
    int index = hash(value);
    Node* current = set->buckets[index];
    Node* previous = NULL;

    while (current != NULL) {
        if (current->data == value) {
            if (previous == NULL) {
                set->buckets[index] = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

// Free the memory allocated for the HashSet
void destroyHashSet(HashSet* set) {
    for (int i = 0; i < HASHSET_SIZE; i++) {
        Node* current = set->buckets[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(set);
}

/*
int main() {
    HashSet* set = initializeHashSet();

    insert(set, 5);
    insert(set, 2);
    insert(set, 10);

    printf("HashSet contains 5: %s\n", contains(set, 5) ? "true" : "false");
    printf("HashSet contains 7: %s\n", contains(set, 7) ? "true" : "false");

    removeValue(set, 2);

    printf("HashSet contains 2 after removal: %s\n", contains(set, 2) ? "true" : "false");

    destroyHashSet(set);

    return 0;
}
*/