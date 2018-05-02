#include "reply.h"
#include <stdio.h>

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
		printf("%d Service closing control connection.\n"
		       "    Logged out if appropriate.\n",
		       reply_code);
	else if (reply_code == 225)
		printf("%d\n", reply_code);
	else if (reply_code == 226)
		printf("%d\n", reply_code);
	else if (reply_code == 227)
		printf("%d\n", reply_code);
	else if (reply_code == 230)
		printf("%d User logged in, proceed.\n", reply_code);
	else if (reply_code == 250)
		printf("%d Requested file action okay, completed.\n",
		       reply_code);
	else if (reply_code == 257)
		printf("%d\n", reply_code);
	else if (reply_code == 331)
		printf("%d User name okay, need password.\n", reply_code);
	else if (reply_code == 332)
		printf("%d Need account for login.\n", reply_code);
	else if (reply_code == 350)
		printf("%d\n", reply_code);
	else if (reply_code == 421)
		printf("%d Service not available, closing control connection.\n"
		       "    This may be a reply to any command if the service "
		       "knows it\n"
		       "    must shut down.\n",
		       reply_code);
	else if (reply_code == 425)
		printf("%d\n", reply_code);
	else if (reply_code == 426)
		printf("%d\n", reply_code);
	else if (reply_code == 450)
		printf("%d\n", reply_code);
	else if (reply_code == 451)
		printf("%d\n", reply_code);
	else if (reply_code == 451)
		printf("%d\n", reply_code);
	else if (reply_code == 500)
		printf("%d Syntax error, command unrecognized.\n"
		       "    This may include errors such as command line too "
		       "long.\n",
		       reply_code);
	else if (reply_code == 501)
		printf("%d Syntax error in parameters or arguments.\n",
		       reply_code);
	else if (reply_code == 502)
		printf("%d\n", reply_code);
	else if (reply_code == 503)
		printf("%d\n", reply_code);
	else if (reply_code == 504)
		printf("%d\n", reply_code);
	else if (reply_code == 530)
		printf("%d Not logged in.\n", reply_code);
	else if (reply_code == 532)
		printf("%d\n", reply_code);
	else if (reply_code == 550)
		printf("%d\n", reply_code);
	else if (reply_code == 551)
		printf("%d\n", reply_code);
	else if (reply_code == 552)
		printf("%d\n", reply_code);
	else if (reply_code == 553)
		printf("%d\n", reply_code);
}
