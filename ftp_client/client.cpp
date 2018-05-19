#include <conio.h>
#include <ctype.h>
#include <iostream>
#include <random>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 1024
#define FTP_PORT "21"
#define FTP_FAIL -1
#define FTP_WIN 0
enum MODE { active, passive };
using namespace std;

// Login to socket with username and password
int handle_login(SOCKET connect_SOCKET);

// Handle user request
int handle_exit(SOCKET connect_SOCKET);
int handle_ls(SOCKET connect_SOCKET, char *input, MODE default_mode);

/* active mode
 * return listen socket
 * wait to accept transfer data
 */
SOCKET mode_active(SOCKET connect_SOCKET);

/* passive mode
 * return socket to transfer data
 */
SOCKET mode_passive(SOCKET connect_SOCKET);

/* receive reply from server
 * return reply code
 */
int recv_reply(SOCKET connect_SOCKET, vector<int> arr_expect);

// check if k exist in arr
int exists_in_arr(vector<int> arr, int k);

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
	status = handle_login(connect_SOCKET);
	if (status == FTP_FAIL) {
		closesocket(connect_SOCKET);
		WSACleanup();
		printf("Login error\n");
		return 0;
	}

	// Read commands from user
	char input[BUFLEN];
	MODE ftp_mode = active;
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
			handle_exit(connect_SOCKET);
			break;
		} else if (input[0] == 'l' && input[1] == 's')
			handle_ls(connect_SOCKET, input, ftp_mode);
		else if (strcmp(input, "active") == 0) {
			if (ftp_mode == passive) {
				ftp_mode = active;
				printf("Switch to active mode\n");
			} else
				printf("Keep current active mode\n");
		} else if (strcmp(input, "pasv") == 0) {
			if (ftp_mode == active) {
				ftp_mode = passive;
				printf("Switch to passive mode\n");
			} else
				printf("Keep current passive mode\n");

		} else
			printf("Command not found. Please type ? or help\n");
	}

	// Cleanup
	closesocket(connect_SOCKET);
	WSACleanup();
	return 0;
}

int handle_login(SOCKET connect_SOCKET)
{
	char buf[BUFLEN]; // buffer receice from or send to ftp serve
	int reply_code;   // reply code from ftp server
	int status;       // for get Winsock error

	// Check connection
	vector<int> arr_reply_code_connect = {220};
	reply_code = recv_reply(connect_SOCKET, arr_reply_code_connect);
	if (!exists_in_arr(arr_reply_code_connect, reply_code))
		return FTP_FAIL;

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
		return FTP_FAIL;
	}

	// Check server recieving username
	vector<int> arr_reply_code_user = {230, 331};
	reply_code = recv_reply(connect_SOCKET, arr_reply_code_user);
	if (!exists_in_arr(arr_reply_code_user, reply_code))
		return FTP_FAIL;

	// If logged in, exit
	if (reply_code == 230) {
		return FTP_WIN;
	}

	// username looks good, need password
	// Get password
	// PASS <SP> <password> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Password: ");
	char password[BUFLEN - 10];
	int temp_i = 0, temp_ch;
	while ((temp_ch = _getch()) != 13) {
		printf("*");
		password[temp_i++] = temp_ch;
	}
	password[temp_i] = '\0';
	fflush(stdin);
	printf("\n");

	// fgets(password, BUFLEN - 10, stdin);
	password[strcspn(password, "\n")] = 0; // remove trailing '\n'
	sprintf_s(buf, "PASS %s\r\n", password);

	// Send password
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return FTP_FAIL;
	}

	// Check server recieving password
	vector<int> arr_reply_code_pass = {230};
	reply_code = recv_reply(connect_SOCKET, arr_reply_code_pass);
	if (!exists_in_arr(arr_reply_code_pass, reply_code))
		return FTP_FAIL;

	return FTP_WIN;
}

int handle_exit(SOCKET connect_SOCKET)
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
	vector<int> arr_reply_code = {221};
	reply_code = recv_reply(connect_SOCKET, arr_reply_code);

	return reply_code;
}

int handle_ls(SOCKET connect_SOCKET, char *input, MODE default_mode)
{
	char buf[BUFLEN];
	int len_temp;
	int reply_code;
	int status;

	SOCKET data_SOCKET = INVALID_SOCKET;
	SOCKET listen_SOCKET = INVALID_SOCKET;
	if (default_mode == active) {
		listen_SOCKET = mode_active(connect_SOCKET);
	} else {
		data_SOCKET = mode_passive(connect_SOCKET);
	}
	// can not create data stream when passive
	if (data_SOCKET == INVALID_SOCKET && listen_SOCKET == INVALID_SOCKET)
		return FTP_FAIL;

	// input co dang "ls [path]"
	// NLST [<SP> <pathname>] <CRLF>
	if (input[2] == '\0') {
		strcpy_s(buf, "NLST\r\n");
	} else {
		strcpy_s(buf, "NLST");
		strcat_s(buf, input + 2);
		strcat_s(buf, "\r\n");
	}

	// send ls msg
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}

	// Check server receiving ls msg
	vector<int> arr_reply_code_ls = {221, 125, 150};
	reply_code = recv_reply(connect_SOCKET, arr_reply_code_ls);
	if (!exists_in_arr(arr_reply_code_ls, reply_code)) {
		closesocket(listen_SOCKET);
		closesocket(data_SOCKET);
		return FTP_FAIL;
	}

	// accept if active
	if (default_mode == active) {
		data_SOCKET = accept(listen_SOCKET, NULL, NULL);
		closesocket(listen_SOCKET);
	}

	// receving data from server
	memset(buf, 0, BUFLEN);
	while ((len_temp = recv(data_SOCKET, buf, BUFLEN, 0)) > 0) {
		printf("%s", buf);
	}

	// Check server successfully transfer
	vector<int> arr_reply_code_suc = {226, 250};
	reply_code = recv_reply(connect_SOCKET, arr_reply_code_suc);
	if (!exists_in_arr(arr_reply_code_suc, reply_code)) {
		closesocket(data_SOCKET);
		return FTP_FAIL;
	}

	closesocket(data_SOCKET);
	return FTP_WIN;
}

SOCKET mode_active(SOCKET connect_SOCKET)
{
	// random port (code from stackoverflow)
	random_device rd;  // obtain a random number from hardware
	mt19937 eng(rd()); // seed the generator
	uniform_int_distribution<> distr(50000, 60000); // define the range
	int ACT_PORT = distr(eng);
	string port_str = to_string(ACT_PORT);

	// Tao listen port
	// Tao addrinfo object
	addrinfo hints, *result, *p_addrinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve server address and port
	int status =
	    getaddrinfo("localhost", port_str.c_str(), &hints, &result);
	if (status != 0) {
		printf("getaddrinfo() failed with error %d\n", status);
		return INVALID_SOCKET;
	}

	// Loop through all the resuls
	char ip_str[INET_ADDRSTRLEN];
	SOCKET listen_SOCKET = INVALID_SOCKET;
	for (p_addrinfo = result; p_addrinfo != NULL;
	     p_addrinfo = p_addrinfo->ai_next) {
		listen_SOCKET =
		    socket(p_addrinfo->ai_family, p_addrinfo->ai_socktype,
			   p_addrinfo->ai_protocol);
		if (listen_SOCKET == INVALID_SOCKET) {
			continue;
		}
		status = bind(listen_SOCKET, p_addrinfo->ai_addr,
			      (int)p_addrinfo->ai_addrlen);
		if (status == SOCKET_ERROR) {
			closesocket(listen_SOCKET);
			continue;
		}
		if (p_addrinfo->ai_family == AF_INET) {
			struct sockaddr_in *ipv4 =
			    (struct sockaddr_in *)p_addrinfo->ai_addr;
			void *addr = &(ipv4->sin_addr);
			inet_ntop(p_addrinfo->ai_family, addr, ip_str,
				  INET_ADDRSTRLEN);
		}
		break;
	}
	freeaddrinfo(result);

	// Listen on a socket
	status = listen(listen_SOCKET, SOMAXCONN);
	if (status == SOCKET_ERROR) {
		printf("listen() failed with error: %d\n", WSAGetLastError());
		closesocket(listen_SOCKET);
		return INVALID_SOCKET;
	}

	// PORT <SP> <host - port> <CRLF>
	// <host - port> :: = <host - number>, <port - number>
	// <host - number> :: = <number>, <number>, <number>, <number>
	// <port - number> :: = <number>, <number>
	// <number> :: = any decimal integer 1 through 255
	for (size_t i = 0; i < strlen(ip_str); ++i) {
		if (ip_str[i] == '.')
			ip_str[i] = ',';
	}
	int port_num_1 = ACT_PORT / 256;
	int port_num_2 = ACT_PORT % 256;
	string msg = "PORT " + string(ip_str) + string(",") +
		     to_string(port_num_1) + string(",") +
		     to_string(port_num_2) + string("\r\n");
	char buf[BUFLEN];
	strcpy_s(buf, msg.c_str());

	// send PORT to server
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		closesocket(listen_SOCKET);
		return INVALID_SOCKET;
	}

	// Get reply from server
	vector<int> arr_reply_code = {200};
	int reply_code = recv_reply(connect_SOCKET, arr_reply_code);
	if (!exists_in_arr(arr_reply_code, reply_code))
		return INVALID_SOCKET;

	return listen_SOCKET;
}

SOCKET mode_passive(SOCKET connect_SOCKET)
{
	char buf[BUFLEN];
	int reply_code;
	int status;

	// PASV <CRLF>
	strcpy_s(buf, "PASV\r\n");
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	memset(buf, 0, BUFLEN);

	// receiving 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
	status = recv(connect_SOCKET, buf, BUFLEN, 0);
	if (status == SOCKET_ERROR) {
		printf("recv() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	// check if 227 correctly
	char *p_end = NULL;
	printf("%s", buf);
	reply_code = strtol(buf, &p_end, 10);
	if (reply_code != 227) {
		return INVALID_SOCKET;
	}
	int h1, h2, h3, h4, p1, p2;
	sscanf_s(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &h1,
		 &h2, &h3, &h4, &p1, &p2);
	// ip number -> string
	string ip_str = to_string(h1) + "." + to_string(h2) + "." +
			to_string(h3) + "." + to_string(h4);
	// port number -> string
	int port_num = p1 * 256 + p2;
	string port_str = to_string(port_num);

	// connect to server by port given
	// Tao addrinfo object
	addrinfo hints, *result, *p_addrinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve server address and port
	status = getaddrinfo(ip_str.c_str(), port_str.c_str(), &hints, &result);
	if (status != 0) {
		printf("getaddrinfo() failed with error %d\n", status);
		return INVALID_SOCKET;
	}

	// Loop through all the resuls and connect the first
	SOCKET data_SOCKET = INVALID_SOCKET;
	for (p_addrinfo = result; p_addrinfo != NULL;
	     p_addrinfo = p_addrinfo->ai_next) {
		data_SOCKET =
		    socket(p_addrinfo->ai_family, p_addrinfo->ai_socktype,
			   p_addrinfo->ai_protocol);
		// invalid_socket -> use next p_addrinfo
		if (data_SOCKET == INVALID_SOCKET) {
			continue;
		}
		status = connect(data_SOCKET, p_addrinfo->ai_addr,
				 (int)p_addrinfo->ai_addrlen);
		// error_socket -> close then use next p_addrinfo
		if (status == SOCKET_ERROR) {
			closesocket(data_SOCKET);
			continue;
		}
		break;
	}

	freeaddrinfo(result);
	return data_SOCKET;
}

int recv_reply(SOCKET connect_SOCKET, vector<int> arr_expect)
{
	char buf[BUFLEN];
	int len_temp;
	int reply_code;
	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		buf[len_temp] = '\0';
		printf("%s", buf);
		reply_code = strtol(buf, &p_end, 10);

		if (exists_in_arr(arr_expect, reply_code)) {
			break;
		}
	}
	return reply_code;
}

int exists_in_arr(vector<int> arr, int k)
{
	for (size_t i = 0; i < arr.size(); ++i) {
		if (arr[i] == k)
			return 1;
	}
	return 0;
}