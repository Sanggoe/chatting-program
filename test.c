#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<dirent.h>
#include<unistd.h>
#include<string.h>


void error();


int main(int argc, char* argv[]) {



    DIR* dp;
    struct dirent* dent;
    struct stat sbuf;
    char path[BUFSIZ];


    if (argc > 4) {

        error();
    }



    if (argc == 1) {

        chdir("..");

        if ((dp = opendir("2015301083")) == NULL) {
            perror("opendir : 2015301083");
        }
        else {
            while ((dent = readdir(dp))) {

                if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;
                printf("%s\t", dent->d_name);
            }

            printf("\n");

        }
        closedir(dp);
    }

    if (argc == 2) {

        if (strcmp(argv[1], "-a") == 0) {
            chdir("..");
            if ((dp = opendir("2015301083")) == NULL) {
                perror("oepndir : 2015301083");
            }
            else {
                while ((dent = readdir(dp))) {

                    printf("%s\t", dent->d_name);

                }
                printf("\n");
            }
            closedir(dp);
        }

        else if (strcmp(argv[1], "-l") == 0) {

            chdir("..");
            if ((dp = opendir("2015301083")) == NULL) {
                perror("opendir : 2015301083");
            }
            else {

                while ((dent = readdir(dp))) {


                    sprintf(path, "2015301083/%s", dent->d_name);
                    stat(path, &sbuf);

                    printf("%d      %s      %d      %d\n", (unsigned int)sbuf.st_mode, dent->d_name, (int)sbuf.st_size, (int)sbuf.st_ino);
                }

            }

        }

        else error();
    }




    if (argc == 3) {




        if ((dp = opendir(argv[1])) == NULL) {
            perror("opendir : subdir");
        }
        else {
            while ((dent = readdir(dp))) {

                printf("%s\t", dent->d_name);

            }
            printf("\n");

        }



    }


    return 0;

}


void error() {

    printf(" 사용 가능 명령어 \n");
    printf(" (공백) \n");
    printf(" -a \n");
    printf(" -l \n");
    printf(" dir \n");
    printf(" -l dir \n");
  
}
