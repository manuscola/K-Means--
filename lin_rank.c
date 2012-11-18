#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>

#define NORMALIZE 

#define FILEPATH "./data_main"
#define MEASURE_MAX
#define BUFSIZE 4096
#define PLAYER_NUM 100
#define NAME_MAX 256
#define MEASURE_DIMENSION 3
typedef enum 
{
    HIT_RATE =1,
    HIT_RATE_3P,
    HIT_RATE_FREE,
    BOARD,
    ASSIST,
    STEAL,
    BLOCK,
    TURNOVER,
    FAULT,
    SCORE
};


typedef struct player
{
    char name[NAME_MAX];
    double measure[MEASURE_DIMENSION];
    int group;
}player;

double ratio[MEASURE_DIMENSION] = {0.0};
double ratio_sqrt[MEASURE_DIMENSION] = {0.0};
double randf(double m)
{
        return m * rand() / (RAND_MAX - 1.);
}
struct player* load_data(const char* path,int player_num)
{
    FILE* fp= NULL;    
    char buf[BUFSIZE] = {0};
    struct player* players = (struct player*) malloc(sizeof(player)*player_num);
    if(players == NULL)
    {
        fprintf(stderr,"malloc failed for players\n");
        return NULL;
    }

    if(access(FILEPATH,R_OK) < 0)
    {
        fprintf(stderr,"can not find the file %s\n",FILEPATH);
        goto err_out;
    }

    fp = fopen(FILEPATH,"rb");
    if(fp == NULL)
    {
        fprintf(stderr,"open file(%s) failed\n",FILEPATH);
        goto err_out;
    }

    int player_index = 0;
    while(fgets(buf,BUFSIZE,fp))
    {
        char *delimit = "\t";
        char *save_ptr;
        char *token=strtok_r(buf,delimit,&save_ptr);

        int field_index = 0;
        while(token != NULL)
        {
            if(field_index == 0)
            {
                strncpy(players[player_index].name,token,NAME_MAX-1);
                players[player_index].name[NAME_MAX-1] = '\0';
            }
            else
            {
                players[player_index].measure[field_index-1] = atof(token);
            }
            token = strtok_r(NULL,delimit,&save_ptr);
            field_index++;
        }
        if(field_index != MEASURE_DIMENSION + 1  )
        {
            fprintf(stderr,"data file have err format ,exit\n" );
            goto err_out;
        }
        player_index++;
        if(player_index == player_num)
        {
            fprintf(stderr,"more than %d players existed in data file\n",player_num);
            break;
        }
    }

    fprintf(stderr,"%d player record got\n",player_index);
    return players;
err_out:
    if(players)
    {
        free(players);
        return NULL;
    }

}

int calc_ratio(struct player* players,int player_num,double* ratio)
{
    int i ;
    double average[MEASURE_DIMENSION] ;
    for (i = 0;i<MEASURE_DIMENSION;i++)
    {
        average[i] = 0.0;
    }
    int j = 0;
    for(i = 0;i<player_num;i++)
    {
        for(j = 0;j<MEASURE_DIMENSION;j++)
        {
            average[j] +=players[i].measure[j];
        }
    }

    for (i = 0;i<MEASURE_DIMENSION;i++)
    {
        average[i]/=player_num;
        ratio_sqrt[i] = 10/average[i];
    }
    for(i = 0; i<player_num;i++)
    {
        for(j = 0;j<MEASURE_DIMENSION;j++)
        {
            players[i].measure[j] = ratio_sqrt[j]*players[i].measure[j];
        }
    }
    return 0;
}

double distance(struct player* player_A,struct player* player_B)
{
    int i = 0;
    double distance = 0.0;
        for(i = 0 ;i<MEASURE_DIMENSION;i++ )
        {
            distance +=pow( (player_A->measure[i] - player_B->measure[i]),2);
        }
    return distance;
}

int nearest(struct player* player, struct player* cent, int n_cluster, double *d2)
{
    int i, min_i;
    struct player* c;
    double d, min_d;

     //for (c = cent, i = 0; i < n_cluster; i++, c++)
     {
        min_d = HUGE_VAL;
        min_i = player->group;
       
        for (c = cent, i = 0; i < n_cluster; i++, c++)
        {
            if (min_d > (d = distance(c, player))) {
                min_d = d; min_i = i;
            }
        }
    }
    if (d2) *d2 = min_d;
    return min_i;
}

void K_findseed(struct player*  players, int player_num, struct player* cent, int n_cent)
{
    int i, j;
    int n_cluster;
    double sum, *d = malloc(sizeof(double) * player_num);

    struct player*  p;
    cent[0] = players[ rand() % player_num ];
    for (n_cluster = 1; n_cluster < n_cent; n_cluster++) {
        sum = 0;
        
       for (j = 0, p = players; j < player_num; j++, p++)
        {
            nearest(p, cent, n_cluster, d + j); 
            sum += d[j];
        }   
        sum = randf(sum);
       for (j = 0, p = players; j < player_num; j++, p++)
        {
            if ((sum -= d[j]) > 0) continue;
            cent[n_cluster] = players[j];
            break;
        }   
    }   
    for (j = 0, p = players; j < player_num; j++, p++)
    {
        p->group = nearest(p, cent, n_cluster, 0); 
    }
    free(d);
}

int K_mean_plus(struct player* players,int player_num,int cluster_num)
{
    struct player* center = malloc(sizeof(player)*cluster_num);
    struct player *p,*c;
    K_findseed(players,player_num,center,cluster_num);
    output_result(players,player_num,cluster_num);
    int i ,j ,min_i;
    int changed;
    do {
        /* group element for centroids are used as counters */
        
        for (c = center, i = 0; i < cluster_num; i++, c++)
        { 
            c->group = 0; 
            for(j = 0;j<MEASURE_DIMENSION;j++)
            {
                c->measure[j] = 0.0;
            }
        }
       
        for (j = 0, p = players; j < player_num; j++, p++)
        {
            c = center+p->group;
            c->group++;
            for(i = 0;i<MEASURE_DIMENSION;i++)
            {
                c->measure[i] += p->measure[i];
            }
        }
       
        for (c = center, i = 0; i < cluster_num; i++, c++)
        {
            for(j = 0;j<MEASURE_DIMENSION;j++)
                c->measure[j]/=c->group;
        }
        changed = 0;
        /* find closest centroid of each point */

        for (j = 0, p = players; j < player_num; j++, p++)
         {
            min_i = nearest(p, center, cluster_num, 0);
            if (min_i != p->group)
            {
                changed++;
                p->group = min_i;
            }
        }
        fprintf(stderr,"%d changed \n",changed);
    } while (changed > 2); /* stop when 99.9% of points are good */

    for (c = center, i = 0; i < cluster_num; i++, c++)
    {   
        fprintf(stderr,"\ncenter %d\n",i);
        for(j = 0;j<MEASURE_DIMENSION;j++)
        #ifdef NORMALIZE
           fprintf(stderr," %lf\t",c->measure[j]/ratio_sqrt[j]);
        #else
           fprintf(stderr," %lf\t",c->measure[j]);
        #endif
    }  
    return 0;
}

int output_result(struct player* players,int player_num,int cluster_num)
{
    int i ,j;
    char cmd[256] = {0};
    struct player *p =players;
    for(i =0 ; i< cluster_num;i++)
    {
        fprintf(stderr,"\nthe group %d\n",i);
        for(j=0,p=players;j<player_num;j++,p++)
        {
            if(p->group == i)
            {
                snprintf(cmd,256,"cat %s |sed -n \"%dp\"",FILEPATH,j+1);
                system(cmd);
               // fprintf(stderr,"%s\t",p->name);
            }
        }
    }

    fprintf(stderr,"\n");
}
int main()
{
    struct player* players = load_data(FILEPATH,100);
    if(players == NULL)
    {
        fprintf(stderr,"load data failed\n");
        return -1;
    }
    int ret = 0;
#ifdef NORMALIZE
    ret = calc_ratio(players,100,ratio);
#endif
    if(ret < 0 )
    {
        fprintf(stderr,"calc ratio failed \n");
        return -2;
    }
 
    ret = K_mean_plus(players,100,8);
    
    ret = output_result(players,100,8);
}
