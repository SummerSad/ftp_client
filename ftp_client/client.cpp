// TODO
// put, get, mput, mget
// mdelete
#include "socket.h"
#include <conio.h>
#include <direct.h>
#include <iostream>
#include <random>
#include <string>

#define BUFLEN 1024
#define FTP_PORT "21"
#define FTP_FAIL -1
#define FTP_WIN 0
#define FTP_EXIT 1
enum MODE { active, passive };
enum CMD_WHERE { single, dual, file };
using namespace std;

struct FTP_CMD {
	string str;
	vector<int> expect_reply;
};

// Login to socket with username and password
int handle_login(SOCKET connect_SOCKET);

/* Change command (cmd)
 * from user input
 * for server to understand
 * example ls -> NLST
 * go_where: single stream, dual stream or file stream
 */
FTP_CMD change_cmd(char *input, CMD_WHERE &go_where);

/* Decide which input go to
 * single stream, dual stream or file stream
 */
int decide_cmd(SOCKET connect_SOCKET, char *input, MODE ftp_mode,
	       char *ip_client_str);

// Handle user request
int handle_cmd_single(SOCKET connect_SOCKET, FTP_CMD cmd);
int handle_cmd_dual(SOCKET connect_SOCKET, FTP_CMD cmd, MODE ftp_mode,
		    char *ip_client_str);
int handle_cmd_file(SOCKET connect_SOCKET, FTP_CMD cmd, MODE ftp_mode,
		    char *ip_client_str);

/* active mode
 * return listen socket
 * wait to accept transfer data
 */
SOCKET mode_active(SOCKET connect_SOCKET, char *ip_client_str);

/* passive mode
 * return socket to transfer data
 */
SOCKET mode_passive(SOCKET connect_SOCKET);

/* receive reply from server
 * return reply code
 */
int recv_reply(SOCKET connect_SOCKET, vector<int> arr_expect);

// check if k exists in arr
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
		exit(1);
	}

	// Tao connect_SOCKET ket noi FTP server
	SOCKET connect_SOCKET = cre_soc_connect(argv[1], FTP_PORT);
	if (connect_SOCKET == INVALID_SOCKET) {
		printf("cre_soc_connect() failed\n");
		WSACleanup();
		exit(1);
	}

	// Get socket info (to get client IP)
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	if (getsockname(connect_SOCKET, (struct sockaddr *)&client_addr,
			&addr_len) < 0) {
		printf("getsockname() error\n");
		WSACleanup();
		exit(1);
	}
	// Get client IP
	char ip_client_str[INET_ADDRSTRLEN];
	if (inet_ntop(client_addr.sin_family, &(client_addr.sin_addr),
		      ip_client_str, sizeof(ip_client_str)) == NULL) {
		printf("inet_ntop() error\n");
		WSACleanup();
		exit(1);
	}

	// Login
	status = handle_login(connect_SOCKET);
	if (status == FTP_FAIL) {
		printf("Login error\n");
		closesocket(connect_SOCKET);
		WSACleanup();
		exit(1);
	}

	// Read commands from user
	char input[BUFLEN];
	MODE ftp_mode = active;
	while (1) {
		printf("ftp> ");
		fgets(input, BUFLEN, stdin);
		input[strcspn(input, "\n")] = 0; // remove trailing '\n'

		// cmd for client
		if (strcmp(input, "active") == 0) {
			if (ftp_mode == active)
				printf("Keep active mode\n");
			else {
				printf("Switch to active mode\n");
				ftp_mode = active;
			}
			continue;
		} else if (strcmp(input, "passive") == 0) {
			if (ftp_mode == passive)
				printf("Keep passive mode\n");
			else {
				printf("Switch to passive mode\n");
				ftp_mode = passive;
			}
			continue;
		}
		// change client directory
		else if (strncmp(input, "lcd", 3) == 0) {
			if (_chdir(input + 4) == -1)
				printf("Fail to change client working "
				       "directory\n");
			else
				printf(
				    "Change client working directory to %s\n",
				    input + 4);
			continue;
		}
		// show current client directory
		else if (strcmp(input, "lpwd") == 0) {
			char *buffer = _getcwd(NULL, 0);
			printf("Current client directory %s\n", buffer);
			free(buffer);
			continue;
		} else if (strcmp(input, "help") == 0 ||
			   strcmp(input, "?") == 0) {
			printf("This is help\n");
			continue;
		}

		// cmd for server
		status =
		    decide_cmd(connect_SOCKET, input, ftp_mode, ip_client_str);
		if (status == FTP_EXIT)
			break;
	}

	// Cleanup
	closesocket(connect_SOCKET);
	WSACleanup();
	return 0;
}

int handle_login(SOCKET connect_SOCKET)
{
	char buf[BUFLEN]; // buffer receice from or send to ftp serve
	int status;       // for get Winsock error

	// Check connection
	vector<int> arr_reply_code_connect = {220};
	if (recv_reply(connect_SOCKET, arr_reply_code_connect) == FTP_FAIL)
		return FTP_FAIL;

	// Username
	// USER <SP> <username> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Username: ");
	char username[BUFLEN - 10];
	fgets(username, BUFLEN - 10, stdin);
	username[strcspn(username, "\n")] = 0; // remove trailing '\n'
	sprintf_s(buf, "USER %s\r\n", username);

	// send username
	status = send(connect_SOCKET, buf, strlen(buf), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return FTP_FAIL;
	}

	// Check server recieving username
	vector<int> arr_reply_code_user = {230, 331};
	if (recv_reply(connect_SOCKET, arr_reply_code_user) == FTP_FAIL)
		return FTP_FAIL;

	// Password
	// PASS <SP> <password> <CRLF>
	memset(buf, 0, BUFLEN);
	printf("Password: ");
	char password[BUFLEN - 10];
	int temp_i = 0, temp_ch;
	// 13 ENTER
	// 8 BACKSPACE
	while ((temp_ch = _getch()) != 13) {
		if (temp_ch == 8) {
			if (temp_i > 0)
				--temp_i;
		} else
			password[temp_i++] = temp_ch;
	}
	password[temp_i] = '\0';
	fflush(stdin);
	printf("\n");
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
	if (recv_reply(connect_SOCKET, arr_reply_code_pass))
		return FTP_FAIL;

	return FTP_WIN;
}

FTP_CMD change_cmd(char *input, CMD_WHERE &go_where)
{
	FTP_CMD cmd;
	if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0 ||
	    strcmp(input, "q") == 0) {
		go_where = single;
		cmd.str = string("QUIT\r\n");
		cmd.expect_reply = {221};
	} else if (strcmp(input, "pwd") == 0) {
		go_where = single;
		cmd.str = string("PWD\r\n");
	} else if (strncmp(input, "cd", 2) == 0) {
		go_where = single;
		string temp = "CWD";
		temp += string(input + 2);
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {250};
	} else if (strncmp(input, "delete", 6) == 0) {
		go_where = single;
		string temp = "DELE";
		temp += string(input + 6);
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {250};
	} else if (strncmp(input, "mkdir", 5) == 0) {
		go_where = single;
		string temp = "MKD";
		temp += string(input + 5);
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {257};
	} else if (strncmp(input, "rmdir", 5) == 0) {
		go_where = single;
		string temp = "RMD";
		temp += string(input + 5);
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {250};
	} else if (strncmp(input, "ls", 2) == 0) {
		go_where = dual;
		string temp = "NLST";
		if (input[2] != '\0') {
			temp += string(input + 2);
		}
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {150};
	} else if (strncmp(input, "dir", 3) == 0) {
		go_where = dual;
		string temp = "LIST";
		if (input[3] != '\0') {
			temp += string(input + 3);
		}
		temp += "\r\n";
		cmd.str = temp;
		cmd.expect_reply = {150};
	} else {
		cmd.str = "_ERROR_";
	}
	return cmd;
}

int decide_cmd(SOCKET connect_SOCKET, char *input, MODE ftp_mode,
	       char *ip_client_str)
{
	CMD_WHERE go_where;
	FTP_CMD cmd = change_cmd(input, go_where);
	if (cmd.str == "_ERROR_") {
		printf("Type help or ?\n");
		return FTP_WIN;
	}
	int status;
	if (go_where == single)
		status = handle_cmd_single(connect_SOCKET, cmd);
	else
		status = handle_cmd_dual(connect_SOCKET, cmd, ftp_mode,
					 ip_client_str);
	return status;
}

int handle_cmd_single(SOCKET connect_SOCKET, FTP_CMD cmd)
{
	// send cmd
	int status = send(connect_SOCKET, cmd.str.c_str(), cmd.str.length(), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
		return FTP_FAIL;
	}

	// receive reply
	status = recv_reply(connect_SOCKET, cmd.expect_reply);

	// only quit have FTP_EXIT
	if (cmd.str == "QUIT\r\n")
		return FTP_EXIT;

	return status;
}

int handle_cmd_dual(SOCKET connect_SOCKET, FTP_CMD cmd, MODE ftp_mode,
		    char *ip_client_str)
{
	int status;

	SOCKET data_SOCKET = INVALID_SOCKET;
	SOCKET listen_SOCKET = INVALID_SOCKET;
	if (ftp_mode == active) {
		listen_SOCKET = mode_active(connect_SOCKET, ip_client_str);
	} else {
		data_SOCKET = mode_passive(connect_SOCKET);
	}

	// can not create data stream or listen socket
	if (data_SOCKET == INVALID_SOCKET && listen_SOCKET == INVALID_SOCKET)
		return FTP_FAIL;

	// send cmd
	status = send(connect_SOCKET, cmd.str.c_str(), cmd.str.length(), 0);
	if (status == SOCKET_ERROR) {
		printf("send() error %d\n", WSAGetLastError());
	}

	// receive reply
	if (recv_reply(connect_SOCKET, cmd.expect_reply) == FTP_FAIL) {
		closesocket(listen_SOCKET);
		closesocket(data_SOCKET);
		return FTP_FAIL;
	}

	// active mode require
	if (ftp_mode == active) {
		data_SOCKET = accept(listen_SOCKET, NULL, NULL);
		closesocket(listen_SOCKET);
	}

	// receving data from server
	char buf[BUFLEN];
	int len_temp;
	memset(buf, 0, BUFLEN);
	while ((len_temp = recv(data_SOCKET, buf, BUFLEN, 0)) > 0) {
		buf[len_temp] = '\0';
		printf("%s", buf);
	}

	// Check server successfully transfer
	vector<int> arr_expect = {226};
	if (recv_reply(connect_SOCKET, arr_expect) == FTP_FAIL) {
		closesocket(data_SOCKET);
		return FTP_FAIL;
	}

	closesocket(data_SOCKET);
	return FTP_WIN;
}

SOCKET mode_active(SOCKET connect_SOCKET, char *ip_client_str)
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
	    getaddrinfo(ip_client_str, port_str.c_str(), &hints, &result);
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
	// 127.0.0.1 -> 127,0,0,1
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
	vector<int> arr_expect = {200};
	if (recv_reply(connect_SOCKET, arr_expect) == FTP_FAIL)
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

	// 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
	status = recv(connect_SOCKET, buf, BUFLEN, 0);
	if (status == SOCKET_ERROR) {
		printf("recv() error %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	} else
		buf[status] = '\0'; // recv return len of buf actual receving

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

	// create data_SOCKET connect to ip and port given
	SOCKET data_SOCKET = cre_soc_connect(ip_str.c_str(), port_str.c_str());
	return data_SOCKET;
}

int recv_reply(SOCKET connect_SOCKET, vector<int> arr_expect)
{
	char buf[BUFLEN];
	int len_temp;
	if ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		buf[len_temp] = '\0';
		printf("%s", buf);
		int reply_code = strtol(buf, &p_end, 10);

		if (!exists_in_arr(arr_expect, reply_code)) {
			return FTP_FAIL;
		}
	}
	return FTP_WIN;
}

int exists_in_arr(vector<int> arr, int k)
{
	for (size_t i = 0; i < arr.size(); ++i) {
		if (arr[i] == k)
			return 1;
	}
	return 0;
}