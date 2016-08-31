#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>

#include "common.h"
#include "log_printf.h"

#include "gdsl.h"
#include "hik_sdk_api.h"
#include "ev.h"

#define H 9
#define L 18
#define SPEED 280000 	// microsecond
#define NODIE 1

char GameMap[H][L];
int key;
int sum=1,over=0;
int dx[4]={0,0,-1,1};
int dy[4]={-1,1,0,0};
struct Snake
{
	int x,y;
	int now;
}Snake[H*L];
const char Shead='@';
const char Sbody='#';
const char Sfood='*';
const char Snode='.';

static struct termios stored_settings;
pthread_t id;

void Initial();
void Create_Food();
void Show();
void Button();
void Move();
void Check_Border();
void Check_Header(int x,int y);
void set_keypress(void);
void reset_keypress(void);
void KeyDetect_thread(void);

int main(int argc, char *argv[])
{
	Initial();
	Show();

	return 0;
}

void Initial()
{
	int i,j;
	int hx,hy;
	int ret;
	//system("title GluttonousSnake");
	memset(GameMap,'.',sizeof(GameMap));
	system("clear");
	srand(time(0));
	hx=rand()%H;
	hy=rand()%L;
	GameMap[hx][hy]=Shead;
	Snake[0].x=hx;
	Snake[0].y=hy;
	Create_Food();

	for(i=0;i<H;i++)
	{
		for(j=0;j<L;j++)
		{
			printf("%c",GameMap[i][j]);
		}
		printf("\n");
	}

	printf("\nA small Game: Gluttonous Snake\n");
	printf("Press any direction key to start...\n");
	set_keypress();
	getchar();
	Button();

	ret=pthread_create(&id,NULL,(void *)KeyDetect_thread,NULL);
	if(ret)
	{
		printf("Create thread error!\n");
		sleep(2);
		exit(0);
	}
}

void KeyDetect_thread(void)
{
	while(1)
	{
		Button();
	}
}

void Button()
{
	key=getchar();
	if(key!=91) return;
	key=getchar();
	switch(key)
	{
		case 68:
			Snake[0].now=0; //left
			//printf("left\n");
			break;
		case 67:
			Snake[0].now=1; //right
			//printf("right\n");
			break;
		case 65:
			Snake[0].now=2; //up
			//printf("up\n");
			break;
		case 66:
			Snake[0].now=3; //down
			//printf("down\n");
			break;
		default:
			break;
	}
}

void Show()
{
	int i, j;
	while (1)
	{
		usleep(SPEED);
		//Button();
		Move();
		if(over)
		{
			printf("\n**Game Over!**\n");
			printf("  >_<\n");
			reset_keypress();
			break;
		}
		system("clear");
		for (i = 0; i < H; i++)
		{
			for (j = 0; j < L; j++)
				printf("%c", GameMap[i][j]);
			printf("\n");
		}

		printf("\nA small Game: Gluttonous Snake\n");
		printf("Press any direction key to start...\n");

	}
}

void Move()
{
	int i,x,y;
	int t=sum;
	x=Snake[0].x;
	y=Snake[0].y;
	GameMap[x][y]='.';
	Snake[0].x=Snake[0].x+dx[Snake[0].now];
	Snake[0].y=Snake[0].y+dy[Snake[0].now];
	Check_Border();
	Check_Header(x,y);
	if(sum==t)
	{
		for(i=1;i<sum;i++)
		{
			if(i==1)
				GameMap[Snake[i].x][Snake[i].y]='.';
			if(i==sum-1)
			{
				Snake[i].x=x;
				Snake[i].y=y;
				Snake[i].now=Snake[0].now;
			}else
			{
				Snake[i].x=Snake[i+1].x;
				Snake[i].y=Snake[i+1].y;
				Snake[i].now=Snake[i+1].now;
			}

			GameMap[Snake[i].x][Snake[i].y]='#';
		}
	}

}

void Check_Header(int x,int y)
{
	if(GameMap[Snake[0].x][Snake[0].y]=='.')
		GameMap[Snake[0].x][Snake[0].y]=Shead;
	else if(GameMap[Snake[0].x][Snake[0].y]=='*')
	{
		GameMap[Snake[0].x][Snake[0].y]=Shead;
		Snake[sum].y=y;
		Snake[sum].x=x;
		Snake[sum].now=Snake[0].now;
		GameMap[Snake[sum].x][Snake[sum].y]=Sbody;
		sum++;
		//if(sum==H*L-10)
		if(sum==20)
		{
			printf("Success! Next stage...\n");
			over=1;
		}
		Create_Food();
	}else
		over = 1;
}

void Check_Border()
{
#ifdef NODIE
	if(Snake[0].x<0)
	{
		Snake[0].x=H-1;
	}else
		Snake[0].x=Snake[0].x%H;

	if(Snake[0].y<0)
		{
			Snake[0].y=L-1;
		}else
			Snake[0].y=Snake[0].y%L;
#else

	if(Snake[0].x<0||Snake[0].x>=H||\
			Snake[0].y<0||Snake[0].y>=L)
	{
		over=1;
		printf("\nSnake cross the border\n");
		printf("Snake x:%d\ty:%d\n",Snake[0].x,Snake[0].y);
	}
#endif

}


void Create_Food()
{
	int fx,fy;
	while(1)
	{
		srand(time(0));
		fx=rand()%H;
		fy=rand()%L;
		if(GameMap[fx][fy]=='.')
		{
			GameMap[fx][fy]=Sfood;
			break;
		}
	}
}

void set_keypress(void)                               //设置终端为RAW模式，并关闭回显
{
    struct termios new_settings;
    tcgetattr(0,&stored_settings);
    new_settings = stored_settings;

    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0,TCSANOW,&new_settings);
    return;
}

void reset_keypress(void)                                //恢复终端属性

{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}
