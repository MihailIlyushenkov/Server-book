#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "telnumbers.h"

// HTable* ht = 0;

const int MAXNAMELENGHT = 100;
const int DEFUSERCOUNT = 1000;


person* person_create(const char* name, const long long unsigned telnum) {
    person* dude = calloc(sizeof(person), 1);
    strncpy(dude->Name, name, MAXNAMELENGHT);
    dude->telnum = telnum;
    return dude;
}

int getdata(person** list_of_people) {
    person* people = calloc(DEFUSERCOUNT, sizeof(person));
    FILE* datafile = fopen("Data", "r");

    char string[100] = {0};
    long long unsigned readedtelnum = 0;
    int usercount = 0;

    for (int i = 0; i < DEFUSERCOUNT; i++) {
        if (fscanf(datafile, "%s %llu", string, &readedtelnum) == 2) {
            strncpy(people[i].Name, string, MAXNAMELENGHT);
            people[i].telnum = readedtelnum;
        }
        else {
            printf("sucsessfully readed %d people's data from book\n", i);
            usercount = i;
            break;
        }
    }

    fclose(datafile);

    *list_of_people = people;
    return usercount;
}

void book_print(person* Book, int Booksize) {
    for (int i = 0; i < Booksize; i++) {
        printf("name: %s, number: %llu\n", (Book[i]).Name, (Book[i]).telnum);
    }
}

void book_write(person* Book, int Booksize) {
    FILE* NewDataFile = fopen("Data_new", "w");
    for (int i = 0; i < Booksize; i++) {
        fprintf(NewDataFile, "%s %llu\n", (Book[i]).Name, (Book[i]).telnum);
    }
    fclose(NewDataFile);
}


size_t person_hash(const void* dude) {
	const person* our_dude = dude;
    const char* c = our_dude->Name;

	size_t h = 0xfea5e33ab78c931eL;
	while(*c) {
		size_t x = *c;
		h ^= x * 0xa17ecd2a19b13a51L;
		h = (h << 41) ^ (h >>23);
		h = (h << 21) ^ (h >>43);
		c++;
	}
	// printf("Hash for: %s:  %lu\n", (const char*)str, h);
	return h;
}

void person_free(void* a) {
    fprintf(stderr, "removed %p\n", a);
    free(a);
}

void* person_copy(const void* source_dude) {
    const person* old_dude = source_dude;
    person* new_dude = calloc(sizeof(person), 1);
    new_dude->telnum = old_dude->telnum;
    strcpy(new_dude->Name, old_dude->Name);

    return new_dude;
}

int person_eq(const void* a, const void* b) {
    const person* a1 = a;
    const person* b1 = b;
    return !strcmp(a1->Name, b1->Name);
}

int my_print(const void* a, size_t h, size_t l, void * data) {
    const person* b = a;
    if (b != 0) fprintf((FILE*) data, "%s %llu\n", b->Name, b->telnum);
    else printf("no such dude");
    return 0;
}

int person_add_to_book(person* book, int* booksize, person* guy) {
    if (book == 0 || booksize == 0 || guy == 0) {
        printf("invalid pointer to adding persons");
        return -1;
    }
    else if (*booksize < 0 || *booksize >= DEFUSERCOUNT) {
        printf("invalid booksize");
        return -2;
    }
    else {
        book[*booksize] = *((person*) person_copy(guy));
        *booksize += 1;
    }
    return 0;
}

//
// int main() {
//     person* list_of_dudes;
//     int count_of_dudes = getdata(&list_of_dudes);
//     printf("book array now is:\n");
//     book_print(list_of_dudes, count_of_dudes);
//     printf("\n");
//
//     ht = ht_init(20, person_hash, person_copy, person_eq, person_free);
//
//     ht_set(ht, &list_of_dudes[0], 7);
//     ht_set(ht, &list_of_dudes[1], 7);
//     ht_set(ht, &list_of_dudes[2], 7);
//     ht_set(ht, &list_of_dudes[3], 7);
//
//     // FILE* file = fopen("Data_new", "w");
//     ht_walk(ht, my_print, stdout);
//     // fclose(file);
//
//     // person* seekable_dude = person_create("Nikitos", 2479633);
//     // person* unknown_dude = 0;
//     // ht_get(ht, seekable_dude, (void**) &unknown_dude);
//
//     // printf("unk dude = %p\n", unknown_dude);
//     // my_print(unknown_dude, 0, 0, stdout);
//
// //     person* newdude = person_create("Nikitosich", 3563453);
// //     person_add_to_book(list_of_dudes, &count_of_dudes, newdude);
// //
// //     book_print(list_of_dudes, count_of_dudes);
//
//     // book_write(list_of_dudes, count_of_dudes);
//     return 0;
// }
