#ifndef _TELNUM_
#define _TELNUM_

typedef struct _person {
    char Name[100];
    long long unsigned telnum;
} person;


person* person_create(const char* name, const long long unsigned telnum);
size_t person_hash(const void* dude);
void person_free(void* a);
void* person_copy(const void* source_dude);
int person_eq(const void* a, const void* b);

int my_print(const void* a, size_t h, size_t l, void * data);
int getdata(person** list_of_people);

#endif //_TELNUM_
