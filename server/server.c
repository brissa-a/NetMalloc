#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 4242

#define READ 1
#define WRITE 2
#define ALLOC 3

// [ID][BEGIN_POINTER][REQUEST]
// [CHAR][UNSIGNED LONG][UNSIGNED LONG]

typedef struct netmalloc {
	unsigned long begin;
	unsigned int size;
	char*pointer;
	struct netmalloc *next;
} netmalloc;

int get_all_data(void *data, int size, int sock) {
	int rec = 0;
	int tmp;
	char *c_data = data;

	while (rec < size) {
		if ((tmp = recv(sock, c_data + rec, size - rec, 0)) < 1)
			return 0;
		rec += tmp;
	}
	return 1;
}

int send_all_data(void *data, int size, int sock) {
	int rec = 0;
	int tmp;
	char *c_data = data;

	while (rec < size) {
		if ((tmp = send(sock, c_data + rec, size - rec, 0)) < 1)
			return 0;
		rec += tmp;
	}
	return 1;
}

void add_new_element(netmalloc **head, unsigned long begin_pointer, unsigned int malloc_size) {
	if (!*head) {
		*head = malloc(sizeof(netmalloc));
		(*head)->pointer = malloc(malloc_size);
		char *init_text = "Guillaume est le meilleur";
		strncpy((*head)->pointer, init_text,
			strlen(init_text) < malloc_size ? strlen(init_text) : malloc_size);
		(*head)->size = malloc_size;
		(*head)->next = 0;
		(*head)->begin = begin_pointer;
		return;
	}
	netmalloc *tmp = *head;
	netmalloc *new = malloc(sizeof(*new));

	while (tmp->next) {
		tmp = tmp->next;
	}
	new->pointer = malloc(malloc_size);
	new->size = malloc_size;
	char *init_text2 = "Non en faite c'est Alexis";
	strncpy(new->pointer, init_text2,
		strlen(init_text2) < malloc_size ? strlen(init_text2) : malloc_size);
	new->next = 0;
	new->begin = begin_pointer;
	tmp->next = new;
}

int handle_write(netmalloc *head, unsigned long begin_pointer, unsigned long from_pointer,
		 unsigned int write_size, int sock) {
	while (head) {
		if (head->begin == begin_pointer)
			break;
		head = head->next;
	}
	if (!head)
		return 1;
	if (!get_all_data(head->pointer + (from_pointer - begin_pointer), write_size, sock))
		return 0;
	return 1;
}

int handle_read(netmalloc *head, unsigned long begin_pointer, unsigned long from_pointer,
		unsigned int read_size, int sock) {
	while (head) {
		if (head->begin == begin_pointer)
			break;
		head = head->next;
	}
	if (!head)
		return 1;
	if (!send_all_data(head->pointer + (from_pointer - begin_pointer), read_size, sock))
		return 0;
	return 1;
}

void handle_client(int client)
{
	netmalloc *head = 0;

	for (;;) {
		char id;
		unsigned long begin_pointer;
		unsigned long from_pointer;
		unsigned int malloc_size;

		printf("Wait id\n");
		if (!get_all_data(&id, sizeof(id), client))
			break;
		printf("Got id: %d\n", id);
		if (id == READ) {
			printf("READ\n");
			if (!get_all_data(&begin_pointer, sizeof(begin_pointer), client))
				break;
			if (!get_all_data(&from_pointer, sizeof(from_pointer), client))
				break;
			if (!get_all_data(&malloc_size, sizeof(malloc_size), client))
				break;
			printf("%lu %lu %d\n", begin_pointer, from_pointer, malloc_size);
			if (!handle_read(head, begin_pointer, from_pointer, malloc_size, client))
				break;
		} else if (id == WRITE) {
			printf("WRITE\n");
			if (!get_all_data(&begin_pointer, sizeof(begin_pointer), client))
				break;
			if (!get_all_data(&from_pointer, sizeof(from_pointer), client))
				break;
			if (!get_all_data(&malloc_size, sizeof(malloc_size), client))
				break;
			if (!handle_write(head, begin_pointer, from_pointer, malloc_size, client))
				break;
		} else if (id == ALLOC) {
			printf("ALLOC\n");
			if (!get_all_data(&begin_pointer, sizeof(begin_pointer), client))
				break;
			printf("begin_pointer received\n", begin_pointer, malloc_size);	
			if (!get_all_data(&malloc_size, sizeof(malloc_size), client))
				break;
			printf("%lu %d\n", begin_pointer, malloc_size);
			add_new_element(&head, begin_pointer, malloc_size);
		}
	}
	while (head) {
		netmalloc *tmp = head->next;
		free(head->pointer);
		free(head);
		head = tmp;
	}
}

int main(void)
{
	struct sockaddr_in sin;
	int sock;
	socklen_t recsize = sizeof(sin);

	struct sockaddr_in csin;
	int csock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);
	if (bind(sock, (struct sockaddr*)&sin, recsize)) {
		perror("bind");
		close(sock);
		return EXIT_FAILURE;
	}

	if (listen(sock, 1)) {
		perror("listen");
		close(sock);
		return EXIT_FAILURE;
	}

	printf("Waiting client\n");
	if ((csock = accept(sock, (struct sockaddr*)&csin, &recsize)) < 0) {
		perror("accept");
		close(sock);
		return EXIT_FAILURE;
	}
	printf("Client connected\n");
	handle_client(csock);
	close(csock);
	close(sock);
	printf("Server is shutting down\n");
	return EXIT_SUCCESS;
}
