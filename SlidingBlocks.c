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

#define MAP_H 21     // must be biger than 11
#define MAP_L 10

#define FALL_SPEED 420000  //MICROSECOND
#define FASTER_SPEED 5000  //MICROSECOND
#define FLASH_TIME 130000

const char no_block = '.';
const char block_body = '#';
const char block_flash = '*';

int key;
int cheat=0;
int speed;
int block_type_count;
int over=0;
int next_block=0;
int point=0;
int has_bomb=0;
int block_attr_changing_flag=0;
char GameField[MAP_H][MAP_L];
static struct termios stored_settings;
pthread_t id;

typedef struct Block
{
	char block_form[4][4];
	int type;
	int x,y;
} s_Block;
s_Block Block;
s_Block Block_Next;

enum block_type
{
	Square=0,
	Strip=1,
	JShape,
	LShape,
	TShape,
	ZShape,
	SShape,
	Bomb,
};

void Initial();
void CreateBlock();
void ShowGame();
void PrintMap();
void CheckPoints();
void KeyDetectThread(void);
void GetButton();

void move_down();
void move_left();
void move_right();
void move_up();
void move_down_faster();

void FillBlockIntoMap();
void RmBlockFromMap();
int BlockCollide(s_Block *block);
void BlockRotate(s_Block *block);
void BlockExplode(int x,int y);

void set_keypress(void);
void reset_keypress(void);

int main(int argc, char *argv[])
{
	Initial();
	ShowGame();
	return 0;
}


void Initial()
{
	int ret;
	int key;

	memset(GameField, no_block, sizeof(GameField));
	set_keypress();
	speed = FALL_SPEED;
	point = 0;
	over = 0;
	has_bomb = 0;
	system("clear");
	printf("\nYou wanna get bomb block randomly? (y/n)\n ");
	key=getchar();
	if(key=='y')
	{
		block_type_count=8;
	}else
		block_type_count=7;
	system("clear");
	CreateBlock();
	Block = Block_Next;
	CreateBlock();
	PrintMap();
	getchar();
	GetButton();

	ret=pthread_create(&id,NULL,(void *)KeyDetectThread,NULL);
	if(ret)
	{
		printf("Create thread error!\n");
		sleep(2);
		exit(0);
	}
}


void CreateBlock()
{
	int b_type;

	memset(Block_Next.block_form,no_block,sizeof(Block_Next.block_form));
	srand(time(0));
	if(!cheat)
	{
		b_type=rand()%block_type_count;
	}else
	{
		b_type = Strip;
		cheat = 0;
	}

	Block_Next.x = 0;
	Block_Next.y = 3;

	switch(b_type)
	{
		case Square:
			Block_Next.block_form[1][1]=block_body;
			Block_Next.block_form[1][2]=block_body;
			Block_Next.block_form[2][1]=block_body;
			Block_Next.block_form[2][2]=block_body;
			Block_Next.type=Square;
			break;
		case Strip:
			Block_Next.block_form[0][1] = block_body;
			Block_Next.block_form[1][1] = block_body;
			Block_Next.block_form[2][1] = block_body;
			Block_Next.block_form[3][1] = block_body;
			Block_Next.type = Strip;
			break;
		case JShape:
			Block_Next.block_form[0][2] = block_body;
			Block_Next.block_form[1][2] = block_body;
			Block_Next.block_form[2][2] = block_body;
			Block_Next.block_form[2][1] = block_body;
			Block_Next.type = JShape;
			break;
		case LShape:
			Block_Next.block_form[0][1] = block_body;
			Block_Next.block_form[1][1] = block_body;
			Block_Next.block_form[2][1] = block_body;
			Block_Next.block_form[2][2] = block_body;
			Block_Next.type = JShape;
			break;
		case TShape:
			Block_Next.block_form[0][1] = block_body;
			Block_Next.block_form[1][1] = block_body;
			Block_Next.block_form[2][1] = block_body;
			Block_Next.block_form[1][2] = block_body;
			Block_Next.type = TShape;
			break;
		case ZShape:
			Block_Next.block_form[0][2] = block_body;
			Block_Next.block_form[1][2] = block_body;
			Block_Next.block_form[1][1] = block_body;
			Block_Next.block_form[2][1] = block_body;
			Block_Next.type = ZShape;
			break;
		case SShape:
			Block_Next.block_form[0][1] = block_body;
			Block_Next.block_form[1][1] = block_body;
			Block_Next.block_form[1][2] = block_body;
			Block_Next.block_form[2][2] = block_body;
			Block_Next.type = SShape;
			break;
		case Bomb:
			Block_Next.block_form[0][2] = block_body;
			Block_Next.block_form[1][1] = block_body;
			Block_Next.block_form[1][3] = block_body;
			Block_Next.block_form[2][2] = block_body;
			Block_Next.type = Bomb;
			break;
		default:
			break;
	}
}


void BlockExplode(int x,int y)
{
	int i,j;

	RmBlockFromMap();
	for (i = x - 1; i <= x + 1; i++)
		for (j = y - 1; j <= y + 1; j++)
		{
			if (i > 3 && i < MAP_H && j >= 0 && j < MAP_L)
				if (GameField[i][j] == block_body)
					GameField[i][j] = block_flash;
		}
	PrintMap();
	usleep(FLASH_TIME);
	for (i = x - 1; i <= x + 1; i++)
		for (j = y - 1; j <= y + 1; j++)
		{
			if (i > 3 && i < MAP_H && j >= 0 && j < MAP_L)
				if (GameField[i][j] == block_flash)
					GameField[i][j] = no_block;
		}
	Block=Block_Next;
	CreateBlock();
	speed=FALL_SPEED;
}

void BlockRotate(s_Block *block)
{
	int i,j;
	char bf[4][4];
	for(i=3;i>=0;i--)
		for(j=0;j<=3;j++)
		{
			bf[j][3-i]=block->block_form[i][j];
		}
	memcpy(block->block_form,bf,sizeof(bf));
}


void move_left()
{
	s_Block t_block;

	t_block=Block;
	t_block.y=t_block.y-1;
	if(BlockCollide(&t_block))
	{
		return;
	}else
	{
		RmBlockFromMap();
		Block=t_block;
		FillBlockIntoMap();
		PrintMap();
	}
}

void move_right()
{
	s_Block t_block;

	t_block = Block;
	t_block.y = t_block.y + 1;
	if (BlockCollide(&t_block))
	{
		return;
	} else
	{
		RmBlockFromMap();
		Block = t_block;
		FillBlockIntoMap();
		PrintMap();
	}
}

void move_down()
{
	s_Block t_block;

	t_block=Block;
	t_block.x=t_block.x+1;
	if(BlockCollide(&t_block))
	{
		if(t_block.x<5)
		{
			over = 1;
			return;
		}else
		{
			next_block = 1;
			speed = FALL_SPEED;
			return;
		}
	}else
	{
		RmBlockFromMap();
		Block=t_block;
		FillBlockIntoMap();
	}
}

void move_down_faster()
{
	speed=FASTER_SPEED;
}


void move_up()  // shift the block
{
	s_Block t_block;

	t_block = Block;
	BlockRotate(&t_block);
	if (BlockCollide(&t_block))
	{
		return;
	} else
	{
		block_attr_changing_flag = 1;
		RmBlockFromMap();
		Block = t_block;
		FillBlockIntoMap();
		PrintMap();
		block_attr_changing_flag = 0;
	}
}

void RmBlockFromMap()
{
	int i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			if (Block.block_form[i][j] == block_body)
				GameField[Block.x + i][Block.y + j] = no_block;
		}
}


int BlockCollide(s_Block *block)
{
	int i,j;
	int xx,yy;

	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
		{
			if (block->block_form[i][j] == block_body)
			{
				if (block->x + i >= MAP_H)
				{
					if (block->type == Bomb)
						BlockExplode(block->x + i, block->y + j);
					return 1;
				}

				if (block->y + j < 0 || block->y + j >= MAP_L)
				{
					if(block->type==Bomb)
						BlockExplode(block->x+i,block->y+j);
					return 1;
				}


				if (GameField[block->x + i][block->y + j] == block_body)
				{
					xx = block->x + i - Block.x;
					yy = block->y + j - Block.y;

					if (0 <= xx && xx < 4 && 0 <= yy && yy < 4)
					{
						if (Block.block_form[xx][yy] == no_block)
						{
							if(block->type==Bomb)
								BlockExplode(block->x+i,block->y+j);
							return 1;
						}
					}else
					{
						if (block->type == Bomb)
							BlockExplode(block->x+i,block->y+j);
						return 1;
					}
				}
			}
		}
	return 0;
}


void ShowGame()
{
	int key;

	while(1)
	{
		usleep(speed);
		if(block_attr_changing_flag)
			continue;
		block_attr_changing_flag=1;
		move_down();
		block_attr_changing_flag=0;
		if(over)
		{
			PrintMap();
			printf("\nGame Over!!!\n");
			printf("Continue? (y/n)\n");
			pthread_cancel(id);
			key=getchar();
			if(key=='y')
			{
				Initial();
				continue;
			}else if(key=='n')
			{
				printf("Exit Game SlidingBlocks...\n");
				reset_keypress();
				exit(0);
			}
		}else if(next_block)
		{
			CheckPoints();
			Block=Block_Next;
			CreateBlock();
			next_block=0;
		}
		PrintMap();
	}
}


void CheckPoints()
{
	int i,j,k;
	int sum=0;		// sum of block body in each line
	int point_ascend = 50;
	int flash_flag=0;
	char t_game_field[MAP_H][MAP_L];

	for (i = MAP_H - 1; i > 3; i--)
	{
		for (j = 0; j < MAP_L; j++)
		{
			if(GameField[i][j]!= block_body) break;
			sum++;
		}
		if(sum==MAP_L)
		{
			for (k = 0; k < MAP_L; k++)
			{
				GameField[i][k] = block_flash;
			}
			flash_flag=1;
			point=point+point_ascend;
			point_ascend=point_ascend+50;
		}
		sum=0;
	}

	if(flash_flag)
	{
		PrintMap();
		usleep(FLASH_TIME);
		memset(t_game_field,no_block,sizeof(GameField));

		k = MAP_H - 1;
		for (i = MAP_H - 1; i > 3; i--)
		{
			if (GameField[i][0] != block_flash)
			{
				for (j = 0; j < MAP_L; j++)
				{
					t_game_field[k][j] = GameField[i][j];
				}
				k--;
			}else
				continue;
		}
		memcpy(GameField,t_game_field,sizeof(GameField));
	}
}


void FillBlockIntoMap()
{
	int i,j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			if (Block.block_form[i][j] == block_body)
				GameField[Block.x + i][Block.y + j] = Block.block_form[i][j];
		}
}


void PrintMap()
{
	int i,j,k;

	system("clear");
	printf("----<SLIDING  BLOCKS>----\n\n");
	for (i = 4; i < MAP_H; i++)
	//for (i = 0; i < MAP_H; i++)
	{
		for (j = 0; j < MAP_L; j++)
		{
			printf("%c ", GameField[i][j]);
		}
		if(i==4)
		{
			printf("\tPOINT : %d",point);
		}

		if (i == 6)
		{
			printf("\tNext Block");
		}

		if(i>=8&&i<=11)
		{
			printf("\t ");
			for(k=0;k<4;k++)
			{
				if(Block_Next.block_form[i-8][k]==no_block)
				{
					printf("  ");
				}else
					printf("%c ",Block_Next.block_form[i-8][k]);
			}
		}
		printf("\n");
	}

	printf("\nA small Game: Sliding Blocks\n");
	printf("Press any key to start...\n");
}


void KeyDetectThread(void)
{
	while(1)
	{
		GetButton();
	}
}

void GetButton()
{
	if (block_attr_changing_flag)
		return;
	key = getchar();
	if (key == 'q')
	{
		cheat = 1;
		return;
	}
	if (key != 91)
		return;
	key = getchar();
	block_attr_changing_flag = 1;
	switch (key)
	{
		case 68:
			move_left();	//left
			break;
		case 67:
			move_right();	//right
			break;
		case 65:
			move_up();		//up
			break;
		case 66:
			move_down_faster();	 //down
			break;
		default:
			break;
	}
	block_attr_changing_flag = 0;
}


void set_keypress(void)       //设置终端为RAW模式，并关闭回显
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

void reset_keypress(void)      //恢复终端属性
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}
