#include"hash.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct _List {
	void* data;
	struct _List* next;
} List;

struct _HTable {
	size_t cap;
	size_t nelem;
	hf hf;
	hcopy hcopy;
	heq heq;
	hfree hfree;
	List** table;
};

HTable* ht_init(size_t initial_sz, hf h, hcopy c, heq eq, hfree f) {
	HTable* n = malloc(sizeof(HTable));
	if ( !n ) return (fprintf(stderr, "Cannot allocate htable\n"), NULL);
	List** tbl = calloc(initial_sz, sizeof(List*));
	if ( !tbl ) return (free(n), fprintf(stderr, "Cannot alocate table\n"), NULL);
	*n = (HTable){initial_sz, 0, h, c, eq, f, tbl};
	return n;
}

void _destroy_list(hfree hf, List* l) {
	if ( !l ) return;
	List* next = l->next;
	hf(l->data);
	free(l);
	_destroy_list(hf, next);
}

void ht_destroy(HTable* ht) {
	size_t i;
	for ( i = 0; i < ht->cap; i++ ) {
		_destroy_list(ht->hfree, ht->table[i]);
	}
	free(ht);
}

int ht_walk(HTable* ht, int (*cb)(const void*, size_t, size_t, void*), void* data) {
	size_t i;
	size_t k;
	List* el;
	for ( i = 0; i < ht->cap; i++ )
		for( el = ht->table[i], k = 0 ; el; el = el->next, k++ )
			if (cb(el->data, i, k, data)) return 1;
	return 0;
}

int ht_get(HTable* ht, const void* key, void** res) {
	size_t h = ht->hf(key) % ht->cap;
	for( List* el = ht->table[h]; el; el = el->next ) {
		if ( ht->heq(el->data, key) ) return (*res = el->data, 1);
	}
	return 0;
}

void _move(List** from_idx, List** to_idx) {
	List* from = *from_idx;
	*from_idx = from->next;
	from->next = *to_idx;
	*to_idx = from;
}

void _rehash(HTable* ht) {
	List** nt  = calloc(ht->cap *= 2, sizeof(List*));
	for ( size_t i = 0; i < ht->cap/2; i++ )
		for (List** el = &ht->table[i]; *el;)
			_move(el, &nt[ht->hf((*el)->data) % ht->cap]);
	free(ht->table);
	ht->table = nt;
}

int ht_set(HTable* ht, const void* pair, size_t max_load_factor) {
	size_t h = ht->hf(pair) % ht->cap;
	size_t load_factor = 0;
	// printf("Set hash %lu\n", h);
	for (List* el = ht->table[h]; el; el = el->next) {
		if (ht->heq(el->data, pair)) return 0;
		load_factor += 1;
	}
	List* n = malloc(sizeof(List));
	*n = (List){ht->hcopy(pair), ht->table[h]};
	ht->table[h] = n;
	ht->nelem++;
	if ( load_factor >= max_load_factor) _rehash(ht);
	return 0;
}

// int ht_del(HTable*, const void* key) {
// 	return -1;
// }

int ht_del(HTable* ht, const void* key) {
	size_t h = ht->hf(key) % ht->cap;
	List* el = ht->table[h];

	if (el == 0) return 0;
	if (ht->heq(el->data, key)) {
		ht->hf(el);
		ht->table[h] = 0;
		return 1;
	}

	List* nxt = el->next;
	for(; nxt; nxt = nxt->next ) {
		if ( ht->heq(nxt->data, key) ) {
			el->next = nxt->next;
			ht->hf(el);
			return 1;
		}
		else el = nxt;
	}
	return 0;
}

int get_lenght(HTable* ht, int index)
{
	int lenght = 0;
	List* list = ht->table[index];
	while (list != 0) {
		list = list->next;
		lenght++;
	}

	return lenght;
}

#define MAX(x, y) (x)>(y)?(x):(y)

int* ht_lght_distr(HTable* ht, int* size) {
	size_t distr_size = 0;

	for (int i = 0; i < ht->cap; i++) {
		distr_size = MAX(distr_size, get_lenght(ht, i));
	}
	distr_size++;
	printf("distr_size = %lu ", distr_size);

	int* distribution = calloc(distr_size, sizeof(int));
	int val = 0;

	for (int i = 0; i < ht->cap; i++) {
		distribution[get_lenght(ht, i)] += 1;
	}

	*size = distr_size;
	return distribution;
}

void ht_data(HTable* ht) {
	printf("ht capacity is %lu, ht nelem is %lu\n", ht->cap, ht->nelem);
}

int ht_elemcount(HTable* ht) {
	return ht->nelem;
}

// int main() {
// 	printf("weg");
// 	return 0;
// }
