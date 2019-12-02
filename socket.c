#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>

#define QUEUE_SIZE 12
#define BUFFER_SIZE 500
#define WAIT_TIME 5

typedef void (*server_process_t)(int);

static char *get_date_time_now_utc()
{
	time_t now = time(NULL);
	if (now == -1)
	{
		printf("ERROR-[get_date_utc]: getting time now with time(NULL)");
		return "";
	}
	struct tm *time_info = gmtime(&now);
	if (time_info == NULL)
	{
		printf("ERROR-[get_date_utc]: time_info is NULL");
		return "";
	}
	char *date_time = (char *)malloc(sizeof(char) * 50);
	strftime(date_time, 50, "%c UTC", time_info);
	return date_time;
}

static void log_info(const struct sockaddr_storage *client_address)
{
	char client_ip[NI_MAXHOST];
	char client_port[NI_MAXSERV];
	socklen_t client_length = sizeof(*client_address);
	int name_info = getnameinfo((struct sockaddr *)(&(*client_address)), client_length, client_ip, sizeof(client_ip), client_port, sizeof(client_port), NI_NUMERICHOST | NI_NUMERICSERV);
	if (name_info != 0)
	{
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(name_info));
		exit(EXIT_FAILURE);
	}
	char *date_time_now = get_date_time_now_utc();
	printf("[INFO]-[%s]-Connection established-[IP:\"%s\"]-[Port:\"%s\"]\n", date_time_now, client_ip, client_port);
	free(date_time_now);
	date_time_now = NULL;
}

static void set_address_reuse(const int *socket_file_descriptor)
{
	const int reuse = 1;
	if (setsockopt(*socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
	{
		printf("ERROR - setsockopt(SO_REUSEADDR) failed\n");
		exit(EXIT_FAILURE);
	}
#ifdef SO_REUSEPORT
	if (setsockopt(*socket_file_descriptor, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("ERROR - setsockopt(SO_REUSEPORT) failed\n");
		exit(EXIT_FAILURE);
	}
#endif
	printf("Socketoption reuse address/port set successfully!\n");
}

static void set_hints(struct addrinfo *hints)
{
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_family = AF_UNSPEC;	 /* Allow IPv4 or IPv6 */
	hints->ai_socktype = SOCK_STREAM; /* Setting TCP */
	hints->ai_flags = AI_PASSIVE;	 /* For wildcard IP address */
	hints->ai_protocol = 0;			  /* Any protocol */
	hints->ai_canonname = NULL;
	hints->ai_addr = NULL;
	hints->ai_next = NULL;
}

static struct addrinfo *get_addresses(const struct addrinfo *hints, const char *port)
{
	struct addrinfo *addresses;
	int address_info = getaddrinfo(NULL, port, hints, &addresses);
	if (address_info != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(address_info));
		exit(EXIT_FAILURE);
	}
	return addresses;
}

static void try_bind_address(struct addrinfo *addresses, int *socket_file_descriptor)
{
	struct addrinfo *address;
	for (address = addresses; address != NULL; address = address->ai_next)
	{
		*socket_file_descriptor = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if ((*socket_file_descriptor) == -1)
		{
			continue;
		}
		set_address_reuse(socket_file_descriptor);
		if (bind((*socket_file_descriptor), address->ai_addr, address->ai_addrlen) == 0)
		{
			printf("Binding succeeded\n");
			break; /* Success */
		}
		close(*socket_file_descriptor);
	}
	if (address == NULL)
	{ /* No address succeeded */
		fprintf(stderr, "Binding failed\n");
		exit(EXIT_FAILURE);
	}
}

static void start_listening(const int *socket_file_descriptor, const char *port)
{
	if (listen((*socket_file_descriptor), QUEUE_SIZE) != 0)
	{
		fprintf(stderr, "Listening failed!\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("Server is listening on port: %s ... \n", port);
	}
}

static const char *get_port(const int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	return argv[1];
}

static void run(const int *socket_file_descriptor, server_process_t server_process)
{
	char buffer[BUFFER_SIZE];
	struct sockaddr_storage client_address;
	int new_socket = -1;
	while (true)
	{
		socklen_t client_length = sizeof(struct sockaddr_storage);
		if ((new_socket = accept(*socket_file_descriptor, (struct sockaddr *)&client_address, &client_length)) < 0)
		{
			fprintf(stderr, "Accept failed with errno: %d\n", errno);
			exit(EXIT_FAILURE);
		}
		log_info(&client_address);
		server_process(new_socket);
		snprintf(buffer, BUFFER_SIZE, "Closing connection in %d seconds ...\n", WAIT_TIME);
		write(new_socket, buffer, strlen(buffer));
		sleep(WAIT_TIME);
		close(new_socket);
	}
}

static void greeting(int file_descriptor)
{
	char buffer[BUFFER_SIZE];
	snprintf(buffer, BUFFER_SIZE, "Hello from Server!\n");
	write(file_descriptor, buffer, strlen(buffer));
}

int main(int argc, char *argv[])
{
	server_process_t server_process = &greeting;
	struct addrinfo hints;
	int socket_file_descriptor = -1;
	const char *port = get_port(argc, argv);
	set_hints(&hints);
	struct addrinfo *addresses = get_addresses(&hints, port);
	try_bind_address(addresses, &socket_file_descriptor);
	freeaddrinfo(addresses);
	start_listening(&socket_file_descriptor, port);
	run(&socket_file_descriptor, server_process);
	return 0;
}
