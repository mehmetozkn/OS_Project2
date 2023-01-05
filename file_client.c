#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

int getIndex(char *data)
{
    // Girilen komutun son elemanının indexini almak için.
    /*  int result = sizeof(data) / sizeof(data[0]);
     return result - 1; */
    int index = 0;
    for (int i = 0; data[i] != '\0'; i++)
    {
        index++;
    }
    return (index - 1);
}

int main()
{
    int fd;
    int exit = 1;
    char *myfifo = "/tmp/myfifo";
    mkfifo(myfifo, 0666);
    char clientStart[128] = "client";
    fd = open(myfifo, O_WRONLY);
    write(fd, clientStart, sizeof(clientStart));
    close(fd);
    printf("---------- PROGRAM KOMUTLARI ----------\n");
    printf("Dosya Olusturmak Icin : create filename\n");
    printf("Dosyaya Veri Yazmak Icin : write filename input\n");
    printf("Dosyadan Veri Okumak Icin : read filename\n");
    printf("Dosyayı Silmek Icin : delete filename\n");
    printf("Programdan Cikis Yapmak Icin : exit\n");
    printf("------------------------------------\n");

    while (exit)
    {

        char input[128];
        char response[128];
        fgets(input, 128, stdin);

        if (input[getIndex(input)] == '\n')
        {
            // Girilen komutun sonundaki aşağı satıra inmesini sağlayan ifadeyi kaldırmak için.
            input[getIndex(input)] = '\0';
        }
        fd = open(myfifo, O_WRONLY);
        write(fd, input, sizeof(input));
        close(fd);
        if (strcmp(input, "exit") == 0)
        {
            // break yapıldığında manager dosyasından çıkış yapılmıyor.
            printf("Clientten Cikis Yapiliyor...");
            exit = 0;
        }
        // Her işlemden sonra cliente response döndürme işlemi
        fd = open(myfifo, O_RDONLY);
        read(fd, response, 128);
        printf("%s\n", response);
    }
    return 0;
}