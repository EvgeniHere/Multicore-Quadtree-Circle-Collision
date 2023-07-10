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
} Hashset;

// Hash function
int hash(int key) {
    return key % HASHSET_SIZE;
}

// Initialize a new Hashset
Hashset* initializeHashSet() {
    Hashset* set = (Hashset*)malloc(sizeof(Hashset));
    for (int i = 0; i < HASHSET_SIZE; i++) {
        set->buckets[i] = NULL;
    }
    return set;
}

// Insert a value into the Hashset
bool insert(Hashset* set, int value) {
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
                return false; // Value already exists, do not insert again
            }
            current = current->next;
        }
        current->next = newNode;
    }
    return true;
}

// Check if a value exists in the Hashset
bool contains(Hashset* set, int value) {
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

// Remove a value from the Hashset
bool removeValue(Hashset* set, int value) {
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
            return true;
        }
        previous = current;
        current = current->next;
    }
    return false;
}

// Free the memory allocated for the Hashset
void destroyHashset(Hashset* set) {
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
    Hashset* set = initializeHashSet();

    insert(set, 5);
    insert(set, 2);
    insert(set, 10);

    printf("Hashset contains 5: %s\n", contains(set, 5) ? "true" : "false");
    printf("Hashset contains 7: %s\n", contains(set, 7) ? "true" : "false");

    removeValue(set, 2);

    printf("Hashset contains 2 after removal: %s\n", contains(set, 2) ? "true" : "false");

    destroyHashset(set);

    return 0;
}
*/