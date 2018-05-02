// TODO hoan thanh print_reply_code (con mot so code chua xong)
// TODO passive mode

#include "reply.h"
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define FTP_PORT "21"
#define BUFLEN 256

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
int handle_ftp_ls(SOCKET connect_SOCKET, char *input);
int handle_ftp_dir(SOCKET connect_SOCKET, char *input);

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
	while (1) {
		printf("ftp> ");
		fgets(input, BUFLEN, stdin);
		input[strcspn(input, "\n")] = 0; // remove trailing '\n'
		if (strcmp(input, "?") == 0 || strcmp(input, "help") == 0)
			printf("quit\texit\t\n"
			       "ls [path]\tdir [path]\n");
		else if (strcmp(input, "quit") == 0 ||
			 strcmp(input, "exit") == 0) {
			handle_ftp_exit(connect_SOCKET);
			break;
		} else if (input[0] == 'l' && input[0] == 's')
			handle_ftp_ls(connect_SOCKET, input);
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
	int len_temp;     // length of buffer receive from or send to ftp server
	int reply_code;   // reply code from ftp server
	int status;       // for get Winsock error
	// Check connection
	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		// 220 Service ready for new user
		if (reply_code == 220) {
			break;
		}
		// error happen
		return 0;
	}

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
	memset(buf, 0, BUFLEN);
	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		// 230 User logged in, proceed
		// 331 User name okay, need password
		if (reply_code == 230 || reply_code == 331) {
			break;
		}
		// error happen
		return 0;
	}

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
	memset(buf, 0, BUFLEN);
	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		// 230 User logged in, proceed
		if (reply_code == 230) {
			break;
		}
		// error happen
		return 0;
	}

	return reply_code;
}

int handle_ftp_exit(SOCKET connect_SOCKET)
{
	char buf[BUFLEN];
	int len_temp;
	int reply_code;
	int status;

	// QUIT <CRLF>
	char quit_msg[] = "QUIT\r\n";
	strncpy_s(buf, quit_msg, strlen(quit_msg));
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}
	memset(buf, 0, BUFLEN);

	// Check server recieving quit msg
	while (len_temp = recv(connect_SOCKET, buf, BUFLEN, 0) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		// 221 Service closing control connection.
		//     Logged out if appropriate.
		if (reply_code == 221) {
			break;
		}
		// error happen
		return 0;
	}

	return reply_code;
}

int handle_ftp_ls(SOCKET connect_SOCKET, char *input)
{
	char buf[BUFLEN];
	int len_temp;
	int reply_code;
	int status;

	// input co dan "ls [path]"
	// NLST [<SP> <pathname>] <CRLF>
	char *p_in = input;
	p_in += 2; // skip "ls"
	while (isspace(*p_in))
		++p_in;
	// only "ls", no path given
	if (*p_in == '\0') {
		char ls_msg[] = "NLST\r\n";
		strncpy_s(buf, ls_msg, strlen(ls_msg));
	} else {
		char ls_msg_begin[] = "NLST ";
		strncpy_s(buf, ls_msg_begin, strlen(ls_msg_begin));
		int i = strlen(ls_msg_begin);
		while (*p_in != '\0') {
			buf[i] = *p_in;
			++i;
			++p_in;
		}
		buf[i] = '\0';
	}
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}
	memset(buf, 0, BUFLEN);

	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		if (reply_code == 221) {
			break;
		}
		// error happen
		return 0;
	}

	return reply_code;
}