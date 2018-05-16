// TODO hoan thanh print_reply_code (con mot so code chua xong)
// TODO passive mode

#include "reply.h"
#include "ulti.h"
#include <iostream>
#include <random>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define FTP_PORT "21"
#define BUFLEN 256
enum MODE { active, passive };
using namespace std;

/* Login to socket with username and password
 * return 0 if fail,
 * otherwise return reply_code (code from server)
 */
int ftp_login(SOCKET connect_SOCKET);

/* Handle user request
 * return 0 if fail,
 * otherwise return reply_code
 */
int handle_ftp_exit(SOCKET connect_SOCKET);
int handle_ftp_ls(SOCKET connect_SOCKET, char *input, MODE default_mode);
// TODO int handle_ftp_dir(SOCKET connect_SOCKET, char *input, MODE
// default_mode);

/* active and passive mode
 * return socket to transfer data
 */
SOCKET mode_ftp_active(SOCKET connect_SOCKET);
SOCKET mode_ftp_passive(SOCKET connect_SOCKET);

/* receive expect reply code
 * return 0 if fail,
 * otherwise return latest reply code
 */
int recv_expect_reply_code(SOCKET connect_SOCKET, int *arr_reply_code,
			   int arr_size);

int main(int argc, char *argv[])
{
	// Kiem tra cu phap
	if (argc != 2) {
		printf("Using: ftp_client ftp.example.com\n");
		exit(1);
	}
	int status;

	// Khoi tao Winsock phien ban 2.2
	WSADATA var_WSADATA;
	status = WSAStartup(MAKEWORD(2, 2), &var_WSADATA);
	if (status != 0) {
		printf("WSAStartup() failed with error: %d\n", status);
		WSACleanup();
		return 1;
	}

	// Tao addrinfo object
	addrinfo hints, *result, *p_addrinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve server address and port
	status = getaddrinfo(argv[1], FTP_PORT, &hints, &result);
	if (status != 0) {
		printf("getaddrinfo() failed with error %d\n", status);
		WSACleanup();
		return 1;
	}

	// Loop through all the resuls and connect the first
	SOCKET connect_SOCKET = INVALID_SOCKET;
	for (p_addrinfo = result; p_addrinfo != NULL;
	     p_addrinfo = p_addrinfo->ai_next) {
		connect_SOCKET =
		    socket(p_addrinfo->ai_family, p_addrinfo->ai_socktype,
			   p_addrinfo->ai_protocol);
		// invalid_socket -> use next p_addrinfo
		if (connect_SOCKET == INVALID_SOCKET) {
			continue;
		}
		status = connect(connect_SOCKET, p_addrinfo->ai_addr,
				 (int)p_addrinfo->ai_addrlen);
		// error_socket -> close then use next p_addrinfo
		if (status == SOCKET_ERROR) {
			closesocket(connect_SOCKET);
			continue;
		}
		if (p_addrinfo->ai_family == AF_INET) {
			char ip4_str[INET_ADDRSTRLEN];
			struct sockaddr_in *ipv4 =
			    +(struct sockaddr_in *)p_addrinfo->ai_addr;
			void *addr = &(ipv4->sin_addr);
			inet_ntop(p_addrinfo->ai_family, addr, ip4_str,
				  INET_ADDRSTRLEN);
			printf("IPv4 %s\n", ip4_str);
		}
		break;
	}

	// done with result
	freeaddrinfo(result);

	// Login
	status = ftp_login(connect_SOCKET);
	if (status == 0) {
		closesocket(connect_SOCKET);
		WSACleanup();
		printf("Login error\n");
		return 0;
	}

	// Read commands from user
	char input[BUFLEN];
	MODE default_mode = active;
	while (1) {
		printf("ftp> ");
		fgets(input, BUFLEN, stdin);
		input[strcspn(input, "\n")] = 0; // remove trailing '\n'
		if (strcmp(input, "?") == 0 || strcmp(input, "help") == 0)
			printf("quit\texit\t\n"
			       "ls [path]\tdir [path]\n");
		else if (strcmp(input, "quit") == 0 ||
			 strcmp(input, "exit") == 0 ||
			 strcmp(input, "q") == 0) {
			handle_ftp_exit(connect_SOCKET);
			break;
		} else if (input[0] == 'l' && input[1] == 's')
			handle_ftp_ls(connect_SOCKET, input, default_mode);
		else
			printf("Command not found. Please type ? or help\n");
	}

	// Cleanup
	closesocket(connect_SOCKET);
	WSACleanup();
	return 0;
}

int ftp_login(SOCKET connect_SOCKET)
{
	char buf[BUFLEN]; // buffer receice from or send to ftp serve
	int reply_code;   // reply code from ftp server
	int status;       // for get Winsock error

	// Check connection
	int arr_reply_code_connect[] = {220};
	int arr_size_connnect =
	    sizeof(arr_reply_code_connect) / sizeof(arr_reply_code_connect[0]);
	reply_code = recv_expect_reply_code(
	    connect_SOCKET, arr_reply_code_connect, arr_size_connnect);

	// Get username
	// USER <SP> <username> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Username: ");
	char username[BUFLEN - 10];
	fgets(username, BUFLEN - 10, stdin);
	username[strcspn(username, "\n")] = 0; // remove trailing '\n'
	sprintf_s(buf, "USER %s\r\n", username);

	// Send username
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}

	// Check server recieving username
	int arr_reply_code_user[] = {230, 331};
	int arr_size_user =
	    sizeof(arr_reply_code_user) / sizeof(arr_reply_code_user[0]);
	reply_code = recv_expect_reply_code(connect_SOCKET, arr_reply_code_user,
					    arr_size_user);

	// If logged in, exit
	if (reply_code == 230) {
		return reply_code;
	}

	// username looks good, need password
	// Get password
	// PASS <SP> <password> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Password: ");
	char password[BUFLEN - 10];
	fgets(password, BUFLEN - 10, stdin);
	password[strcspn(password, "\n")] = 0; // remove trailing '\n'
	sprintf_s(buf, "PASS %s\r\n", password);

	// Send password
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}

	// Check server recieving password
	int arr_reply_code_pass[] = {230};
	int arr_size_pass =
	    sizeof(arr_reply_code_pass) / sizeof(arr_reply_code_pass[0]);
	reply_code = recv_expect_reply_code(connect_SOCKET, arr_reply_code_pass,
					    arr_size_pass);

	return reply_code;
}

int handle_ftp_exit(SOCKET connect_SOCKET)
{
	char buf[BUFLEN];
	int reply_code;
	int status;

	// QUIT <CRLF>
	strcpy_s(buf, "QUIT\r\n");
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}
	memset(buf, 0, BUFLEN);

	// Check server recieving quit msg
	int arr_reply_code[] = {221};
	int arr_size = sizeof(arr_reply_code) / sizeof(arr_reply_code[0]);
	reply_code =
	    recv_expect_reply_code(connect_SOCKET, arr_reply_code, arr_size);

	return reply_code;
}

int handle_ftp_ls(SOCKET connect_SOCKET, char *input, MODE default_mode)
{
	char buf[BUFLEN];
	int reply_code;
	int status;

	SOCKET data_SOCKET;
	if (default_mode == active) {
		data_SOCKET = mode_ftp_active(connect_SOCKET);
	} else {
		data_SOCKET = mode_ftp_passive(connect_SOCKET);
	}

	// input co dang "ls [path]"
	// NLST [<SP> <pathname>] <CRLF>
	if (input[2] == '\0') {
		strcpy_s(buf, "NLST\r\n");
	}
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}

	// Check server receiving ls msg
	int arr_reply_code[] = {221};
	int arr_size = sizeof(arr_reply_code) / sizeof(arr_reply_code[0]);
	reply_code =
	    recv_expect_reply_code(connect_SOCKET, arr_reply_code, arr_size);

	closesocket(data_SOCKET);
	return reply_code;
}

SOCKET mode_ftp_active(SOCKET connect_SOCKET)
{
	// random port (code from stackoverflow)
	std::random_device rd;  // obtain a random number from hardware
	std::mt19937 eng(rd()); // seed the generator
	std::uniform_int_distribution<> distr(50000, 60000); // define the range
	int ACT_PORT = distr(eng);

	char ip4_str[999];

	// PORT <SP> <host - port> <CRLF>
	// <host - port> :: = <host - number>, <port - number>
	// <host - number> :: = <number>, <number>, <number>, <number>
	// <port - number> :: = <number>, <number>
	// <number> :: = any decimal integer 1 through 255
	int port_num_1 = ACT_PORT / 256;
	int port_num_2 = ACT_PORT % 256;
	string port_msg = "PORT " + string(ip4_str) + string(",") +
			  to_string(port_num_1) + string(",") +
			  to_string(port_num_2) + string("\r\n");

	int arr_reply_code[] = {200};
	int arr_size = sizeof(arr_reply_code) / sizeof(arr_reply_code[0]);
	int reply_code =
	    recv_expect_reply_code(connect_SOCKET, arr_reply_code, arr_size);

	return 0;
}

SOCKET mode_ftp_passive(SOCKET connect_SOCKET)
{
	return 0;
}

int recv_expect_reply_code(SOCKET connect_SOCKET, int *arr_reply_code,
			   int arr_size)
{
	char buf[BUFLEN];
	int len_temp;
	int reply_code;
	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		if (exists_in_arr(arr_reply_code, arr_size, reply_code)) {
			break;
		}
		// error happen
		return 0;
	}
	return reply_code;
}