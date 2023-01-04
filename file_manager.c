#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_BUF 128

FILE *fl;
char file_list[10][100];
char response[128];
char c;
int id = 0;
int file_list_length = sizeof(file_list) / sizeof(file_list[0]);
int number_of_clients = 0;
pthread_t threads[4];
pthread_mutex_t mutex_lock;
pthread_cond_t lock_cond;

struct params
{
    char *arg1;
    char *arg2;
};

int checkFileList(char *);
void *createFile(char *);
void *deleteFile(char *);
void *readFile(char *);
void *writeFile(char *);
char **arraySplit(char *);
char **matrixGenerate(int, int);

int main()
{
    int fd;
    char **inputs_array;
    void *status;
    int trueInput = -1;
    char *myfifo = "/tmp/myfifo";
    char buf[MAX_BUF];
    memset(file_list, '\0', sizeof(file_list)); // Arraydeki çöp değerleri temizleyerek hata çıkmasını engellemek için.

    pthread_mutex_init(&mutex_lock, NULL);
    pthread_cond_init(&lock_cond, NULL);

    while (1)
    {
        fd = open(myfifo, O_RDONLY);
        read(fd, buf, MAX_BUF);

        inputs_array = arraySplit(buf);

        struct params params;

        params.arg1 = inputs_array[1];
        params.arg2 = inputs_array[2];

        if (!strcmp(inputs_array[0], "client"))
        {
            // Açılan clientlerin sayısını tutar.
            number_of_clients++;
        }
        else if (!strcmp(inputs_array[0], "create"))
        {
            // file_list’te dosya ismi yoksa boş olan sıraya dosya ismini ekler ve sistemde dosyayı oluşturur.
            pthread_create(&threads[0], NULL, createFile, &params);
            trueInput = 1;
        }
        else if (!strcmp(inputs_array[0], "delete"))
        {
            // file_list’te dosya ismi varsa sistemden ve File_List’ten(indexten) dosyayı siler.
            pthread_create(&threads[1], NULL, deleteFile, &params);
            trueInput = 1;
        }
        else if (!strcmp(inputs_array[0], "write"))
        {
            // file_list’te dosya ismi varsa File_client’tan gelen datayı dosyaya yazar.
            pthread_create(&threads[2], NULL, writeFile, &params);
            trueInput = 1;
        }
        else if (!strcmp(inputs_array[0], "read"))
        {
            // file_list’te dosya ismi varsa dosyayı okur ve ekrana yazdırır.
            pthread_create(&threads[3], NULL, readFile, &params);
            trueInput = 1;
        }
        else if (!strcmp(inputs_array[0], "exit"))
        {
            // Ilgili client threadini bitirir, iletişim kopar.
            number_of_clients--;

            if (number_of_clients == 0)
            {
                fd = open(myfifo, O_WRONLY);
                write(fd, response, sizeof(response));
                close(fd);
                strcpy(response, "Programdan Cikis Yapiliyor...");
                exit(0);
            }
            trueInput = 1;
        }
        else
        {
            trueInput = 0;
            strcpy(response, "Yanlis Komut Girdiniz! Tekrar Deneyiniz.");
        }

        for (int i = 0; i < 4; i++)
        {
            pthread_join(threads[i], &status);
        }

        if (trueInput == 1 || trueInput == 0)
        {
            fd = open(myfifo, O_WRONLY);
            write(fd, response, sizeof(response));
            trueInput = -1;
            close(fd);
        }
    }

    pthread_mutex_destroy(&mutex_lock);
    pthread_cond_destroy(&lock_cond);
    exit(0);
}

int checkFileList(char *file_name)
{
    // File Listesinde boş olan indexi bulmak için.
    // -61 sayısı, indexde olmaması için yani herhangi bir negatif sayı olabilir.
    int result = -61;
    for (int i = 0; i < file_list_length; i++)
    {
        if (file_list[i] != NULL)
        {
            if (strcmp(file_list[i], file_name) == 0)
            {
                result = i;
                break;
            }
        }
    }

    return result;
}

void *createFile(char *args)
{
    pthread_mutex_lock(&mutex_lock); // Thread lock.
    // Kulanıcı tarafından girilen inputları ayırma işlemleri.
    struct params *params = args;
    char *file_name = params->arg1;

    int result = checkFileList(file_name);
    if (result == -61)
    {
        for (int i = 0; i < 10; i++)
        {
            if (file_list[i][0] == '\0')
            {
                strcpy(file_list[i], file_name);
                FILE *file;
                file = fopen(file_name, "w");
                fclose(file);
                strcpy(response, "Dosya Olusturuldu.");
                break;
            }
        }
    }
    else
    {
        // Dosya bulunamazsa client tarafına bunun bilgisini göndermek için.
        strcpy(response, "Dosya zaten var!");
    }
    pthread_mutex_unlock(&mutex_lock); // Thread unlock.
}

void *deleteFile(char *args)
{

    // Dosya silme işlemi.
    pthread_mutex_lock(&mutex_lock); // Thread lock.
                                     // Kulanıcı tarafından girilen inputları ayırma işlemleri.
    struct params *params = args;
    char *file_name = params->arg1;

    int result = checkFileList(file_name);

    if (result != -61)
    {

        file_list[result][0] = '\0';
        remove(file_name);
        strcpy(response, "Dosya Silindi.");
    }
    else
    {
        // Dosya bulunamazsa client tarafına bunun bilgisini göndermek için.
        strcpy(response, "Silinecek dosya bulunmadı!");
    }
    pthread_mutex_unlock(&mutex_lock);
}

void *readFile(char *args)
{
    // Dosyadan okuma işlemi.
    pthread_mutex_lock(&mutex_lock); // Thread lock.
    // Kulanıcı tarafından girilen inputları ayırma işlemleri.
    struct params *params = args;
    char *file_name = params->arg1;
    char *okunanData = malloc(128 * sizeof(char));

    int result = checkFileList(file_name);

    if (result != -61)
    {
        FILE *file;
        char ch;

        file = fopen(file_name, "r+");
        if (file == NULL)
        {
            strcpy(response, "Dosya Acilamadi!");
        }
        else
        {

            ch = fgetc(file);
            int i = 0;
            while (ch != EOF)
            {
                *(okunanData + i) = ch;
                i++;
                ch = fgetc(file);
            }
            *(okunanData + i - 1) = '\0';
        }

        fclose(file);

        strcpy(response, okunanData);
    }
    else
    {
        // Dosya bulunamazsa client tarafına bunun bilgisini göndermek için.
        strcpy(response, "Okunacak Dosya Bulunamadı!");
    }
    pthread_mutex_unlock(&mutex_lock); // Threadh unlock.
}

void *writeFile(char *args)
{
    // Dosyaya yazma işlemi
    pthread_mutex_lock(&mutex_lock); // Thread lock.
    // Kulanıcı tarafından girilen inputları ayırma işlemleri.
    struct params *params = args;
    char *file_name = params->arg1;
    char *data = params->arg2;

    int result = checkFileList(file_name);

    if (result != -61)
    {

        FILE *file = fopen(file_name, "a+");
        if (file == NULL)
        {
            strcpy(response, "Dosya Acilamadi!");
        }

        fprintf(file, "%s\n", data);

        if (fclose(file) == EOF)
        {
            strcpy(response, "Dosya Acilirken Hata Olustu!");
        }
        strcpy(response, "Dosyaya Yazıldı.");
    }
    else
    { // Dosya bulunamazsa client tarafına bunun bilgisini göndermek için.
        strcpy(response, "Yazılacak Dosya Bulunamadı!");
    }
    pthread_mutex_unlock(&mutex_lock); // Thread unlock.
}

char **matrixGenerate(int row, int column)
{
    // Hafızada array için yer ayırtan fonksiyon. BP3 dersinde aynısını projede yapmıştım. Kaynakçada mevcut.
    int i;
    char **matrix = malloc(row * sizeof(int *));
    for (i = 0; i < row; i++)
    {
        matrix[i] = malloc(column * sizeof(int));
    }

    return matrix;
}

char **arraySplit(char *array)
{
    // İnputu ayırma işlemi.
    int index = 0;
    char *tokenize = strtok(array, " ");
    char **resultArray;
    resultArray = matrixGenerate(10, 10);
    while (tokenize != NULL)
    {
        *(resultArray + index) = tokenize;
        index++;
        tokenize = strtok(NULL, " ");
    }

    return resultArray;
}