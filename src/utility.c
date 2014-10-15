/***************************
 *文件名称：utility.c
 *功能描述：杂项
 *作    者：LYC
 *创建日期：2013-04-10
 *编码格式：utf-8
 **************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utility.h"

/*
	30: 黑 31: 红 32: 绿  33: 黄 34: 蓝  35: 紫  36: 深绿 37: 白色 
	none = "\033[0m"
	black = "\033[0;30m"
	dark_gray = "\033[1;30m"
	
	颜色函数
*/

const char *none="\033[0m";
const char *red="\033[0;31m";
const char *blue="\033[0;34m";
const char *yellow="\033[0;33m";

static void color_set(char *src, char *dest, const char *color)
{
	dest[0] = '\0';
	strcat(dest, color);
	strcat(dest, src);
	strcat(dest, none);
}


void color_red(char *src, char *dest)
{
	color_set(src, dest, red);
}


void color_blue(char *src, char *dest)
{
	color_set(src, dest, blue);
}


void color_yellow(char *src, char *dest)
{
	color_set(src, dest, yellow);
}
