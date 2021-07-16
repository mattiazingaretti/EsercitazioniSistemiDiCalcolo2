#include <unistd.h>
#include <errno.h>
#include "common.h"


int readOneByOne(int fd, char* buf, char separator) {

    int ret, bytes_read = 0;

    /** [TO DO] READ THE MESSAGE THROUGH THE FIFO DESCRIPTOR
     *
     * Suggestions:
     * - you can read from a FIFO as from a regular file descriptor
     * - since you don't know the length of the message, just
     *   read one byte at a time from the socket
     * - leave the cycle when 'separator' ('\n') is encountered 
     * - repeat the read() when interrupted before reading any data
     * - return the number of bytes read
     * - reading 0 bytes means that the other process has closed
     *   the FIFO unexpectedly: this is an error that should be
     *   dealt with!
     **/
     char curr; 
     //TODO CHECK il -1 
     while(1){
		
		ret = read(fd , buf + bytes_read , 1);
		
		if(ret == 0) handle_error("Error in readOneByOne FIFO close unexpectedly \n");
		if(ret == -1){ if (errno == EINTR) continue; handle_error("Error in readOneByOne ! \n");}
		
		curr = buf[bytes_read]; 
		bytes_read += ret;
		if(curr == separator) break;
	 }
	 //printf("Read %d bytes \n ", bytes_read); //DEBUG
	return bytes_read;
}

void writeMsg(int fd, char* buf, int size) {

    int ret,bytes_left , bytes_sent;
    /** [TO DO] SEND THE MESSAGE THROUGH THE FIFO DESCRIPTOR
     *
     * Suggestions:
     * - you can write on the FIFO as on a regular file descriptor
     * - make sure that all the bytes have been written: use a while
     *   cycle in the implementation as we did for file descriptors!
     **/
    bytes_left = strlen(buf);
    bytes_sent = 0;
	
	while(bytes_left > 0 ){
		
		ret = write(fd, buf+bytes_sent , bytes_left);
		
		if(ret == -1){
			if(errno == EINTR ) continue;
			handle_error("Error in writing welcome message on ECHO fifo\n ");
		}
		bytes_sent += ret;
		bytes_left -= ret;
	}
	//printf("Sent message of size : %d\n ", size);DEBUG
}
