// TODO hoan thanh print_reply_code (con mot so code chua xong)
// TODO nhap password ma khong hien len terminal

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define FTP_PORT "21"
#define BUFLEN 256

/* Login to socket with username and password
 * return reply_code (code send from server)
 */
int ftp_login(SOCKET connect_SOCKET);

/* Print full message from server
 * by reply_code, follow RFC 959
 */
void print_reply_code(int reply_code);

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
		return reply_code;
	}
	memset(buf, 0, BUFLEN);

	// Get username
	// USER <SP> <username> <CRLF>
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
	memset(buf, 0, BUFLEN);

	// Check server recieving username
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
		return reply_code;
	}
	memset(buf, 0, BUFLEN);

	// If logged in, exit
	if (reply_code == 230) {
		return reply_code;
	}

	// username looks good, need password
	// Get password
	// PASS <SP> <password> <CRLF>
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
	memset(buf, 0, BUFLEN);

	// Check server recieving password
	while ((len_temp = recv(connect_SOCKET, buf, BUFLEN, 0)) > 0) {
		char *p_end = NULL;
		reply_code = strtol(buf, &p_end, 10);
		print_reply_code(reply_code);

		// 230 User logged in, proceed
		if (reply_code == 230) {
			break;
		}
		// error happen
		return reply_code;
	}
	memset(buf, 0, BUFLEN);

	return reply_code;
}

void print_reply_code(int reply_code)
{
	if (reply_code == 110)
		printf(
		    "%d Restart marker reply.\n"
		    "    In this case, the text is exact and not left to the\n"
		    "    particular implementation; it must read :\n"
		    "    \tMARK yyyy = mmmm\n"
		    "    Where yyyy is User - process data stream marker, and "
		    "mmmm\n"
		    "    server's equivalent marker (note the spaces between "
		    "markers\n"
		    "    and \"=\").\n",
		    reply_code);

	else if (reply_code == 120)
		printf("%d Service ready in nnn minutes.\n", reply_code);
	else if (reply_code == 125)
		printf("%d Data connection already open; transfer starting.\n",
		       reply_code);
	else if (reply_code == 150)
		printf("%d File status okay; about to open data connection.\n",
		       reply_code);
	else if (reply_code == 200)
		printf("%d Command okay.\n", reply_code);
	else if (reply_code == 202)
		printf(
		    "%d Command not implemented, superfluous at this site.\n",
		    reply_code);
	else if (reply_code == 211)
		printf("%d System status, or system help reply.\n", reply_code);
	else if (reply_code == 212)
		printf("%d Directory status.\n", reply_code);
	else if (reply_code == 213)
		printf("%d File status.\n", reply_code);
	else if (reply_code == 214)
		printf("%d Help message.\n"
		       "    On how to use the server or the meaning of a "
		       "particular\n"
		       "    non - standard command.This reply is useful only "
		       "to the\n"
		       "    human user.\n",
		       reply_code);
	else if (reply_code == 215)
		printf("%d NAME system type.\n"
		       "    Where NAME is an official system name from the "
		       "list in the\n"
		       "    Assigned Numbers document.\n",
		       reply_code);
	else if (reply_code == 220)
		printf("%d Service ready for new user.\n", reply_code);
	else if (reply_code == 221)
		;
	else if (reply_code == 225)
		;
	else if (reply_code == 226)
		;
	else if (reply_code == 227)
		;
	else if (reply_code == 230)
		printf("%d User logged in, proceed.\n", reply_code);
	else if (reply_code == 250)
		printf("%d Requested file action okay, completed.\n",
		       reply_code);
	else if (reply_code == 257)
		;
	else if (reply_code == 331)
		printf("%d User name okay, need password.\n", reply_code);
	else if (reply_code == 332)
		printf("%d Need account for login.\n", reply_code);
	else if (reply_code == 350)
		;
	else if (reply_code == 421)
		printf("%d Service not available, closing control connection.\n"
		       "    This may be a reply to any command if the service "
		       "knows it\n"
		       "    must shut down.\n",
		       reply_code);
	else if (reply_code == 425)
		;
	else if (reply_code == 426)
		;
	else if (reply_code == 450)
		;
	else if (reply_code == 451)
		;
	else if (reply_code == 451)
		;
	else if (reply_code == 500)
		printf("%d Syntax error, command unrecognized.\n"
		       "    This may include errors such as command line too "
		       "long.\n",
		       reply_code);
	else if (reply_code == 501)
		printf("%d Syntax error in parameters or arguments.\n",
		       reply_code);
	else if (reply_code == 502)
		;
	else if (reply_code == 503)
		;
	else if (reply_code == 504)
		;
	else if (reply_code == 530)
		printf("%d Not logged in.\n", reply_code);
	else if (reply_code == 532)
		;
	else if (reply_code == 550)
		;
	else if (reply_code == 551)
		;
	else if (reply_code == 552)
		;
	else if (reply_code == 553)
		;
}