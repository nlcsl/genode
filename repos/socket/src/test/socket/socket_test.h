
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
 
#define SERVER "10.0.2.1"
#define BUFLEN 512  //Max length of buffer
#define PORT 10000  //The port on which to send data
 

int socket_test(void)
{
    struct sockaddr_in si_other;
    int socket_fd, i;
	unsigned int slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN] = "abcd";
 
    if ( (socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
		printf("socket error: errno: %d\n", errno );
		return -1;
    }

	// read fcntl flags on socket_fd 
	int flags = fcntl(socket_fd, F_GETFL);
	printf("fcntl flags: %x \n", flags );

	// set fcntl O_NONBLOCK flag on socket_fd
	flags |= O_NONBLOCK;
	fcntl(socket_fd, F_SETFL, flags);
 
	int numfd = socket_fd + 1;

	// prepare fd_set
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(socket_fd, &read_fds);

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
     
    if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
    {
        fprintf(stderr, "inet_aton() failed\n");
		return -1;
    }
 
	for (unsigned i = 0; i<10; i++)
    {
        
        //send the message
        if (sendto(socket_fd, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
			printf("sendto error: errno: %d\n", errno );
			return -1;
        }
        
		printf("before select\n"); 
		int select_result = select(numfd, &read_fds, NULL, NULL, NULL);
		printf("select_result: %x\n", select_result );

		if (select_result == -1) {
			printf("select error: errno: %d\n", errno );
			return -1;
		}

        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(socket_fd, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
			printf("recvfrom error: errno: %d\n", errno );
			return -1;
        }
         
        puts(buf);
    }
 
    return 0;
}
