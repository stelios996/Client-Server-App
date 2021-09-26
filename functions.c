#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include "functions.h"

void init_pool(struct pool_t *pool){

    pool->b_count = 0;
    pool->b_start = 0;
    pool->b_end = -1;
}

void writer_lock(){

    pthread_mutex_lock(&mtx);
    while(writers>0 || readers>0 || pool.b_count==MAXFILES)
        pthread_cond_wait(&writer_cond, &mtx);
    writers++;
    pthread_mutex_unlock(&mtx);
}

void writer_unlock(){

    pthread_mutex_lock(&mtx);
    writers--;
    pthread_cond_broadcast(&reader_cond);
    pthread_cond_broadcast(&writer_cond);
    pthread_mutex_unlock(&mtx);
}

void reader_lock(){

    pthread_mutex_lock(&mtx);
    while(writers>0 || readers>0 || pool.b_count==0)
        pthread_cond_wait(&reader_cond, &mtx);
    readers++;
    pthread_mutex_unlock(&mtx);
}

void reader_unlock(){

    pthread_mutex_lock(&mtx);
    readers--;
    pthread_cond_broadcast(&writer_cond);
    pthread_cond_broadcast(&reader_cond);
    pthread_mutex_unlock(&mtx);
}


void list_dir(const char * dir_name, char *lbuffer){		//sunarthsh apo to https://www.lemoda.net/c/recursive-directory/ thn opoia tropopoihsa

    char buff[MAXSIZE];
    DIR * d;
    d = opendir (dir_name);     //anoigw ton fakelo dir_name
    if (! d) {                  //elegxw an anoi3e
        fprintf (stderr, "Cannot open directory '%s': %s\n", dir_name, strerror (errno));
        exit(EXIT_FAILURE);
    }
    while (1) {
        struct dirent * entry;
        const char * d_name;
        entry = readdir (d);    //pairnw thn epomenh kataxwrhsh tou d
        if (! entry) {
            break;              //den uparxoun alles kataxwrhseis opote break
        }
        d_name = entry->d_name;
        if(strcmp (d_name, "..") != 0 && strcmp (d_name, ".") != 0){
            sprintf(buff, "%s/%s\n", dir_name, d_name);     //bazw to path ths kataxwrhshs ston lbuffer
            strcat(lbuffer, buff);
        }

        if (entry->d_type & DT_DIR) {       //an einai fakelos
            if (strcmp (d_name, "..") != 0 && strcmp (d_name, ".") != 0) {
                int path_length;
                char path[PATH_MAX];

                path_length = snprintf (path, PATH_MAX, "%s/%s", dir_name, d_name); //to path tou fakelou
                if (path_length >= PATH_MAX) {
                    fprintf (stderr, "Path length has got too long.\n");
                    exit (EXIT_FAILURE);
                }
                list_dir(path, lbuffer);    //kalw thn idia sunarthsh gia to neo fakelo
            }
        }
    }
    if (closedir (d)) {     //kleinw to fakelo
        fprintf (stderr, "Could not close '%s': %s\n", dir_name, strerror (errno));
        exit(EXIT_FAILURE);
    }

}
