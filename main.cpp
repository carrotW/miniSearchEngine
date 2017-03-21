//
//  main.cpp
//  mini search engine
//
//  Created by 蒋晨书 on 2017/3/16.
//  Copyright © 2017年 蒋晨书. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct article
{
    char *name;
    int frequency;
    struct article *next;
};

struct term
{
    char * word;                        /*记录了这个term的单词名*/
    struct article * articleList;       /*记录了包含这个term的所有文章项*/
    struct term * next;
};

struct index
{
    bool isEstablished;
    struct term * termList;
};
struct index Index;

/*暂时使用单项链表实现，之后会修改成B树*/
void build(char *abuf,int alength,char * buf,int length)
{
    if(Index.termList == NULL)
    {
        /*建立单词链表，并建立第一项*/
        /*链表第一项为空项*/
        Index.termList = (struct term *)malloc(sizeof(struct term));
        Index.termList->word = NULL;
        Index.termList->articleList = NULL;
        /*链表正真的第一项建立*/
        Index.termList->next = (struct term *)malloc(sizeof(struct term));
        Index.termList->next->word = buf;
        /*文章链表第一项为空项*/
        Index.termList->next->articleList = (struct article *)malloc(sizeof(struct article));
        Index.termList->next->articleList->name = NULL;
        Index.termList->next->articleList->frequency = -1;
        Index.termList->next->articleList->next = NULL;
        /*文章第二项为真正的第一项*/
        Index.termList->next->articleList->next =(struct article *)malloc(sizeof(struct article));
        Index.termList->next->articleList->next->frequency = 1;
        Index.termList->next->articleList->next->name = abuf;
        Index.termList->next->articleList->next->next = NULL;
        /*index和第一个单词项建立完毕*/
    }
    else
    {
        /*分为3种情况，能找到单词项->能找到对应单词项的文章项（次数+1和不能找到对应单词项的文章项(需要插入新的文章项),和不能找到单词项*/
        struct term *tmpTerm;
        tmpTerm = Index.termList->next;
        while(tmpTerm!=NULL)
        {
            /*进入循环，比较每个存在的单词项*/
            int result;
            result = strcmp(buf,tmpTerm->word);
            if(result != 0) /*此单词项不是所需要的单词项*/
            {
                tmpTerm = tmpTerm->next;
                continue;
            }
            else /*找到已存在的对应单词项*/
            {
                /*判断能不能找到对应的文章项*/
                struct article *tmpArticle;
                tmpArticle = tmpTerm->articleList->next; /*记录文章链表第一项的地址,用于判断循环*/
                while(tmpArticle != NULL)
                {
                    int aResult;
                    aResult = strcmp(abuf,tmpArticle->name);
                    if(aResult != 0)
                    {
                        tmpArticle = tmpArticle->next;
                        continue;
                    }
                    else /*找到对应的文章项,frequency加1*/
                    {
                        tmpArticle->frequency++;
                        return ;
                    }
                }
                /*跳出while循环，说明没有找到对应的文章项，需要自己建立*/
                /*在开头插入文章项*/
                tmpArticle = tmpTerm->articleList->next;
                tmpTerm->articleList->next = (struct article*)malloc(sizeof(struct article));
                tmpTerm->articleList->next->frequency = 1;
                tmpTerm->articleList->next->name = abuf;
                tmpTerm->articleList->next->next = tmpArticle;
                return ;
            }
        }
        /*跳出了while循环，说明没有找到对应的单词项，需要自己建立*/
        /*在开头插入单词项*/
        tmpTerm = Index.termList->next;
        Index.termList->next = (struct term *)malloc(sizeof(struct term));
        Index.termList->next->next = tmpTerm; /*将新插入的单词项连接上后面的链表*/
        Index.termList->next->word = buf;
        Index.termList->next->articleList = NULL;
        /*在此新单词项插入第一个空项作为链表的头*/
        Index.termList->next->articleList = (struct article *)malloc(sizeof(struct article));
        Index.termList->next->articleList->name = NULL;
        Index.termList->next->articleList->frequency = -1;
        Index.termList->next->articleList->next = NULL;
        /*在此新单词项插入第一个真正的项*/
        Index.termList->next->articleList->next =(struct article *)malloc(sizeof(struct article));
        Index.termList->next->articleList->next->frequency = 1;
        Index.termList->next->articleList->next->name = abuf;
        Index.termList->next->articleList->next->next = NULL;
        return;
        
    }
    return ;
}

/*读取文章并建造index*/
void read(char * abuf,int alength,FILE *fpa)
{
    char buf[40]; /*假设每个单词最多39位*/
    memset(buf,0,40);
    int position=0;
    int size=0;
    size = fread(buf, 1, 1, fpa); /*size==0，说明读到了文件末尾，否则说明读到了字符*/
    while(size != 0)
    {
        if((buf[position] != ' ')&&(position == 0))
        {
            /*如果不是空格，并且position等于0,说明读入了单词的第一个字符*/
            /*position加1，再读入字符*/
            position++;
            size = fread(buf+position, 1, 1, fpa);
            if(size == 0)
                build(abuf,alength,buf,position);
        }
        else if((buf[position] != ' ')&&(position != 0))
        {
            /*如果不是空格，并且position不等于0，说明已经读入了单词的第一个字符，并又读入了字符*/
            /*position加1，buf再读入字符*/
            /*未完成，判断是否是噪声词*/
            position++;
            size = fread(buf+position, 1, 1, fpa);
            /*如果size为0，进行文章的最后一次build*/
            if(size == 0)
                build(abuf,alength,buf,position);
        }
        else if((buf[position] == ' ')&&(position != 0))
        {
            /*如果是空格，并且position不等于0,说明读入了单词，并读到了单词末尾的空格*/
            /*进行build步骤,build后清空buf，position赋值0，再读入字符*/
            /*未完成，判断是否是噪声词*/
            build(abuf,alength,buf,position);
            position = 0;
        }
        else if((buf[position] == ' ')&&(position == 0))
        {
            /*如果是空格，并且position等于0,说明还未读入第一个单词，并读到了空格*/
            /*一切不变，只需文件指针移动重新读*/
            size = fread(buf,1,1,fpa); /*position继续为0，buf清空*/
        }
        /*如果size为0，则说明读到了文件末尾，循环结束*/
    }
    return;
}

int main() {
    Index.isEstablished = false;
    Index.termList = NULL;
    char * buf;
    buf = NULL;
    buf = (char *)malloc(40*sizeof(char));
    /*建立一个用于搜索的index项*/
    FILE *fp;
    fp = fopen("/Users/jiangchenshu/Downloads/shakespeareWhole/index.txt", "r");
    while(fgets(buf,39,fp) != NULL) /*读完所有的文章*/
    {
        buf[strlen(buf)-1]='\0';    /*去掉末尾的换行符'\n',此时文章标题的length减1*/
        FILE *fpa;
        fpa = fopen(buf,"r");       /*打开对应的文章*/
        read(buf,strlen(buf),fpa);
    }
    /*建立完毕，可以写搜索的函数用于检测是否有bug*/
    return 0;
}
