#include<stdio.h>
#include<uv.h>
#include<stdlib.h>
#include<string.h>

#include "hash.h"
#include "telnumbers.h"

uv_loop_t* loop;
HTable* ht;

typedef struct _bufstack {
	size_t capacity; //count of elements
	size_t size; //size in bytes
	uv_buf_t* buf_array;
} bufstack;

int backup_count = 0;
int max_buf_count = 1000;
int clients = 0;
int write_database(HTable* ht, const char* filename);

void on_conn_write(uv_write_t* req, int status) {
	if ( status ) {
		fprintf(stderr, "Error while writing: %s\n", uv_strerror(status));
	}
	free(uv_req_get_data((uv_req_t*)req));
	free(req);
}


void alloc_buffer(uv_handle_t* client, size_t sz, uv_buf_t* buf) {
	buf->base = malloc(sz);
	buf->len = sz;
}

void on_conn_close(uv_handle_t* client) {
	char buff[100];
	snprintf(buff, 100, "data_backup_%d", backup_count);
	backup_count++;
	write_database(ht, buff);
	fprintf(stderr, "Connection closed\n");
	clients--;
	fprintf(stderr, "Clients: %d\n", clients);
}

#define BUFFSIZE 105
void create_wbuf_from_person(const person* dude, uv_buf_t* wbuf) {
	if (dude == 0) {
		wbuf->base = strdup("there is no such person in the table\n");
		wbuf->len = strlen("there is no such person in the table\n");
	}
	else {
		wbuf->base = calloc(BUFFSIZE, 1);
		wbuf->len = snprintf(wbuf->base, BUFFSIZE,"%s : %llu\n", dude->Name, dude->telnum);;
	}
}

person* create_person_from_strings(const char* name, const char* telnum) {
	if (name == 0) return 0;

	person* dude = malloc(sizeof(person));
	strncpy(dude->Name, name, 100);

	if (telnum != 0) sscanf(telnum, "%llu", &(dude->telnum));
	else dude->telnum = 0;

	return dude;
}


int write_database(HTable* ht, const char* filename) {
	if (filename == 0) {
		ht_walk(ht, my_print, stdout);
	}
	else {
		FILE* file = fopen(filename, "w");
		ht_walk(ht, my_print, file);
		fclose(file);
	}
	return 0;
}

int add_buf_to_stk(bufstack* stk, uv_buf_t buf) {
	if (stk->capacity >= max_buf_count) return fprintf(stderr, "errror: buffer stack overflow, some data will be missed");

	if (stk->buf_array == 0) {
		fprintf(stderr, "error: empty bufer data, cant copy it");
		return 2;
	}
	stk->buf_array[stk->capacity].base = strdup(buf.base);
	stk->buf_array[stk->capacity].len = buf.len;

	stk->capacity += 1;
	stk->size += buf.len;

	return 0;
}

int arr_setter(const void* a, size_t h, size_t l, void* data) {
    const person* b = a;
	bufstack* stk = data;
	uv_buf_t wbuf;
	create_wbuf_from_person(b, &wbuf);
	add_buf_to_stk(stk, wbuf);
	return 0;
}


void write_database_to_port(HTable* ht, bufstack* stk) {
	int nelem = ht_elemcount(ht);
	if (nelem == 0) {

		stk->buf_array[0].base = strdup("empty database");
		stk->buf_array[0].len = strlen("empty database");

		stk->capacity = 1;
		stk->size = strlen("empty database");
	}
	else {
		stk->buf_array = calloc(sizeof(uv_buf_t)*nelem, 1);
		ht_walk(ht, arr_setter, (void*) stk);
	}
}


void invalid_input(bufstack* stk) {
	uv_buf_t wbuf;
	fprintf(stderr, "invalid input");
	wbuf.base = strdup("invalid input\n");
	wbuf.len = strlen("invalid input\n");
	add_buf_to_stk(stk, wbuf);
}

uv_buf_t create_buf_from_string(const char* str) {
	uv_buf_t wbuf;
	wbuf.base = strdup(str);
	wbuf.len = strlen(str);
	return wbuf;
}

void stk_print(bufstack* stk) {
	for (int i = 0; i < stk->capacity; i++) {
		fprintf(stderr, "%d -> %s\n", i, stk->buf_array[i].base);
	}
}

int parse_input(const char* buff, size_t nread, bufstack* stk) {
	uv_buf_t wbuf;

	if (buff == 0) {
		fprintf(stderr, "fatal error - void input\n");
		return -1;
	}
	// if (nread == 2) {
	// 	invalid_input(stk);
	// }

	char* my_buff = calloc(nread*sizeof(char), 1);
	memcpy(my_buff, buff, nread);

	my_buff[nread-1] = 0; //удаление \r и \n из конца буфера (костыль)
	my_buff[nread-2] = 0;

	// for (int i = 0; i < nread; i++) {
	// 	printf("my_buff[%d] = <%c = %d>\n", i, my_buff[i], my_buff[i]);
	// }

	char string1[100] = {0};
	char string2[100] = {0};
	char string3[100] = {0};
	char string4[100] = {0};
	char string5[100] = {0};
	int scans = sscanf(my_buff, "%s %s %s %s", string1, string2, string3, string4);


	// fprintf(stderr, "nread = %lu, buff: <%s>,\n scans = <%d>:\n \t1:<%s>\n \t2:<%s>\n \t3:<%s>\n \t4:<%s>\n", nread, my_buff, scans, string1, string2, string3, string4);


	const char* string_to_write;

	if (scans == 0 || scans == -1) {
		invalid_input(stk);
	}
	else if (scans == 1) {
		if (strcmp(string1, "dmp") == 0) {
			write_database_to_port(ht, stk);
			wbuf = create_buf_from_string("\nsucessfully dumped\n");
			add_buf_to_stk(stk, wbuf);
		}
		else if (strcmp(string1, "quit") == 0) {
			return -1;
		}
		else invalid_input(stk);
	}
	else if (scans == 2) {
		if (strcmp(string1, "get") == 0) {
		//find person
			person* seek_tar = create_person_from_strings(string2, 0);
			person* founded = 0;
			ht_get(ht, seek_tar, (void**) &founded);
			create_wbuf_from_person(founded, &wbuf);
			add_buf_to_stk(stk, wbuf);
			person_free(seek_tar);
		}
		else if (strcmp(string1, "rem") == 0) {
		//remove person
			person* del_tar = create_person_from_strings(string2, 0);
			if (ht_del(ht, del_tar)) {
				wbuf = create_buf_from_string("\nsucessfully removed\n");
				add_buf_to_stk(stk, wbuf);
			}
			else {
				create_wbuf_from_person(0, &wbuf);
				add_buf_to_stk(stk, wbuf);
			}
		}
		else invalid_input(stk);
	}
	else if (scans == 3) {
		if (strcmp(string1, "add") == 0) {
		// add person
			person* add_tar = create_person_from_strings(string2, string3);
			ht_set(ht, add_tar, 7);
			wbuf = create_buf_from_string("\nsuccessfully added\n");
			add_buf_to_stk(stk, wbuf);
			person_free(add_tar);
		}
		else if (strcmp(string1, "rew") == 0) {
			//rewrite person = remove + add
			person* rew_tar = create_person_from_strings(string2, string3);
			if (ht_del(ht, rew_tar)) {
				ht_set(ht, rew_tar, 7);
				wbuf = create_buf_from_string("\nsucessfully rewritted\n");
				add_buf_to_stk(stk, wbuf);
			}
			else {
				create_wbuf_from_person(0, &wbuf);
				add_buf_to_stk(stk, wbuf);
			}
			person_free(rew_tar);
		}
		else invalid_input(stk);
	}
	else invalid_input(stk);

	return 0;
}


void on_client_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
	if ( nread <  0)  {
		if ( nread != UV_EOF ) {
			fprintf(stderr, "Client read error %s\n", uv_err_name(nread));
		} else {
			fprintf(stderr, "Client closed connection\n");
		}
		free(buf->base);
		uv_close((uv_handle_t*)client, on_conn_close);
	} else if ( nread > 0 ) {
		uv_write_t* req = malloc(sizeof(uv_write_t));
		/* allocating new buf for writing */
		bufstack stk;
		stk.buf_array = calloc(sizeof(uv_buf_t), 10);
		stk.capacity = 0;
		stk.size = 0;

		int result = parse_input(buf->base, nread, &stk);
		free(buf->base);

		if (result == -1) {
			uv_close((uv_handle_t*)client, on_conn_close);
		}
		else {
			uv_req_set_data((uv_req_t*)req, stk.buf_array);
			uv_write(req, client, stk.buf_array, stk.capacity, on_conn_write);
		}
	}
}

void on_new_connection(uv_stream_t* server, int status) {
	if (status < 0) {
		fprintf(stderr, "New connection error: %s\n", uv_strerror(status));
		return;
	}
	uv_tcp_t* client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);
	if ( uv_accept(server, (uv_stream_t*)client) == 0 ) {
		clients++;
		fprintf(stderr, "Clients: %d\n", clients);

		uv_write_t* req = malloc(sizeof(uv_write_t));
		uv_buf_t wbuf;
		const char * welcome_string = "команды:\n\tget <name>			- ищет в таблице человека с заданным именем, выводит телефонный номер\n\tadd <name> <telnumber> 		- добавляет человека в таблицу с заданным именем и номером\n\trem <name> 			- удаляет человека по имени\n\trew <name> <telnumber> 		- меняет телефонный номер человека на заданный\n\tdmp <filename (optional)>	- сбрасывает базу на диск (если нет аргументов, то в консоль)\n";
		int lenght = strlen(welcome_string);
		wbuf.base = strndup(welcome_string, lenght);
		wbuf.len = lenght;

		uv_req_set_data((uv_req_t*)req, wbuf.base);
		uv_write(req, (uv_stream_t*) client, &wbuf, 1, on_conn_write);

		uv_read_start((uv_stream_t*)client, alloc_buffer, on_client_read);
	} else {
		uv_close((uv_handle_t*)client, on_conn_close);
	}
}

// gcc -o asyncsrv asyncsrv.c -luv
// ctrl + z
// bg
// telnet localhost 8989

/*
	команды:
		get <name> 					- ищет в таблице человека с заданным именем, выводит телефонный номер
		add <name> <telnumber> 		- добавляет человека в таблицу с заданным именем и номером
		rem <name> 					- удаляет человека по имени
		rew <name> <telnumber> 		- меняет телефонный номер человека на заданный
		dmp <filename (optional)>	- сбрасывает базу на диск
*/

int main() {
	uv_tcp_t server;
	struct sockaddr_in addr;

	ht = ht_init(20, person_hash, person_copy, person_eq, person_free);
	person* list_of_people = 0;
	int people_count = getdata(&list_of_people);

	for(int i = 0; i < people_count; i++) {
		ht_set(ht, &(list_of_people[i]), 5);
	}
	free(list_of_people);

	loop = uv_default_loop();

	uv_tcp_init(loop, &server);
	uv_ip4_addr("127.0.0.1", 8989, &addr);
	uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
	int result = uv_listen((uv_stream_t*)&server, 1024, on_new_connection);
	if ( result ) return -fprintf(stderr, "Cannot listen, %s\n", uv_strerror(result));
	result = uv_run(loop, UV_RUN_DEFAULT);
	ht_destroy(ht);
	return result;
}

