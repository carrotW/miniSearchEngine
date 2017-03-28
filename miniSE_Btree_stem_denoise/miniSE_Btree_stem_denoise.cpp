//
//  main.cpp
//  miniSearchEngine
//
//  Created by 蒋晨书 on 2017/3/24.
//  Copyright © 2017年 蒋晨书. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>  /* for isupper, tolower */
#include <time.h>

#include "./libstemmer_c/include/libstemmer.h"
const char * progname;
static int pretty = 1;
char * noise[100];
static void stem_file(struct sb_stemmer * stemmer, FILE * f_in, FILE * f_out) {
#define INC 10
	int lim = INC;
	sb_symbol * b = (sb_symbol *) malloc(lim * sizeof(sb_symbol));

	while (1) {
		int ch = getc(f_in);
		if (ch == EOF) {
			free(b);
			return;
		}
		{
			int i = 0;
			int inlen = 0;
			while (1) {
				if (ch == '\n' || ch == EOF)
					break;
				if (i == lim) {
					sb_symbol * newb;
					newb = (sb_symbol *) realloc(b,
							(lim + INC) * sizeof(sb_symbol));
					if (newb == 0)
						goto error;
					b = newb;
					lim = lim + INC;
				}
				/* Update count of utf-8 characters. */
				if (ch < 0x80 || ch > 0xBF)
					inlen += 1;
				/* force lower case: */
				if (isupper(ch))
					ch = tolower(ch);

				b[i] = ch;
				i++;
				ch = getc(f_in);
			}

			{
				const sb_symbol * stemmed = sb_stemmer_stem(stemmer, b, i);
				if (stemmed == NULL) {
					fprintf(stderr, "Out of memory");
					exit(1);
				} else {
					if (pretty == 1) {
						fwrite(b, i, 1, f_out);
						fputs(" -> ", f_out);
					} else if (pretty == 2) {
						fwrite(b, i, 1, f_out);
						if (sb_stemmer_length(stemmer) > 0) {
							int j;
							if (inlen < 30) {
								for (j = 30 - inlen; j > 0; j--)
									fputs(" ", f_out);
							} else {
								fputs("\n", f_out);
								for (j = 30; j > 0; j--)
									fputs(" ", f_out);
							}
						}
					}

					fputs((char *) stemmed, f_out);
					putc('\n', f_out);
				}
			}
		}
	}
	error: if (b != 0)
		free(b);
	return;
}

struct article {
	int ID;        //文章ID
	int frequency;
	struct article *next;
};

struct term {
	char * word; /*记录了这个term的单词名*/
	int frequency; /*含有这个词的文章数量*/
	struct article * articleList; /*记录了包含这个term的所有文章项*/
};

struct vNode {
	bool isLeaf;
	struct vNode * parent; /*如果parent为NULL，则此结点为根结点*/
	struct node * nodeData; /*if node is leaf, this item is NULL*/
	struct leaf * leafData; /*if node is not leaf, this item is NULL*/
};

struct node {
	struct vNode * child1;
	struct vNode * child2;
	struct vNode * child3;
	char * key1;
	char * key2;
};
struct leaf {
	struct term * term[3];

};

struct index {
	bool isEstablished;
	struct vNode * root;
};

struct index Index;
char forDebug;
int debugcount = 0;

int articleID = 0;
int position = 0; /*表示非叶结点中的child或者叶结点中的term位置*/

struct vNode * find(struct vNode * root, char * key) {
	struct vNode *tmp = NULL;
	/*如果根结点不存在，直接返回NULL*/
	if (Index.root == NULL)
		return NULL;
	/*如果这个结点是叶结点，返回tmp结点（也就是该结点），并改变全局变量position，示意key这个字符串是否已经存在*/
	if (root->isLeaf == true) {
		tmp = root;
		/*对选定的vNode进行判断*/
		/*vNode里面有对应的项,返回*/
		int result;
		result = strcmp(tmp->leafData->term[0]->word, key);
		if (result == 0)
			position = 1;
		else if (tmp->leafData->term[1] == NULL
				|| strcmp(tmp->leafData->term[1]->word, key) > 0)
			position = 2;
		else if (strcmp(tmp->leafData->term[1]->word, key) == 0)
			position = 3;
		else if (tmp->leafData->term[2] == NULL
				|| strcmp(tmp->leafData->term[2]->word, key) > 0)
			position = 4;
		else if (strcmp(tmp->leafData->term[2]->word, key) == 0)
			position = 5;
		else if (strcmp(tmp->leafData->term[2]->word, key) < 0)
			position = 6;
		else
			printf("position refresh failed!\n");
//		else {
//			result = strcmp(tmp->leafData->term[1]->word, key);
//			if (result == 0)
//				position = 3;
//			else if (result > 0)
//				position = 2;
//			else {
//				result = strcmp(tmp->leafData->term[2]->word, key);
//				if (result > 0)
//					position = 4;
//				else if (result == 0)
//					position = 5;
//				else
//					position = 6;
//			}
//		}
	}
	/*如果这个结点不是叶结点，进行判断，等待返回tmp*/
	else {
		/*第二个key不为空,对key1和key2都进行判断，选下一个结点，进入下一次find*/
		if (root->nodeData->key2 != NULL) {
			int compareResult1 = strcmp(key, root->nodeData->key1);
			int compareResult2 = strcmp(key, root->nodeData->key2);
			if (compareResult1 < 0)
				tmp = find(root->nodeData->child1, key);
			else if (compareResult1 >= 0 && compareResult2 < 0)
				tmp = find(root->nodeData->child2, key);
			else
				tmp = find(root->nodeData->child3, key);
		}
		/*第二个key为空，只需对key1进行判断,选下一个结点，进入下一次find*/
		else {
			int compareResult1 = strcmp(key, root->nodeData->key1);
			if (compareResult1 < 0)
				tmp = find(root->nodeData->child1, key);
			else
				tmp = find(root->nodeData->child2, key);
		}
	}

	return tmp;
	/*未找到对应的项,*/
}

/*分裂结点*/
void split(char * key, struct vNode * tmpvNode, struct vNode *child1,
		struct vNode *child2) {
	struct vNode * parent;
	parent = tmpvNode->parent;

	/*要分裂的结点是根结点*/
	if (parent == NULL) {
		/*如果此结点是根结点且是叶结点，父结点为NULL,直接申请一个新vNode作为root，并新申请一个vNode作为另一个分裂叶结点*/
		if (tmpvNode->isLeaf == true) {
			parent = (struct vNode *) malloc(sizeof(struct vNode));
			parent->isLeaf = false;
			parent->leafData = NULL;
			parent->parent = NULL;
			parent->nodeData = (struct node *) malloc(sizeof(struct node));
			parent->nodeData->child1 = NULL; /*需要赋值*/
			parent->nodeData->child2 = NULL; /*需要赋值*/
			parent->nodeData->child3 = NULL;
			parent->nodeData->key1 = NULL; /*需要赋值*/
			parent->nodeData->key2 = NULL;
			tmpvNode->parent = parent;
			Index.root = parent;
			struct vNode * newvNode = (struct vNode *) malloc(
					sizeof(struct vNode));
			newvNode->isLeaf = true;
			newvNode->nodeData = NULL;
			newvNode->parent = parent;
			newvNode->leafData = (struct leaf *) malloc(sizeof(struct leaf));
			newvNode->leafData->term[0] = NULL; /*需要赋值*/
			newvNode->leafData->term[1] = NULL; /*需要赋值*/
			newvNode->leafData->term[2] = NULL;
			struct term * newTerm = (struct term *) malloc(sizeof(struct term));
			newTerm->word = key;
			newTerm->frequency = 1;
			newTerm->articleList = (struct article *) malloc(
					sizeof(struct article));
			newTerm->articleList->frequency = 1;
			newTerm->articleList->ID = articleID;
			newTerm->articleList->next = NULL;
			struct term * tmpTerm;
			if (position == 2) {
				tmpTerm = tmpvNode->leafData->term[1];
				tmpvNode->leafData->term[1] = newTerm;

				newvNode->leafData->term[0] = tmpTerm;
				newvNode->leafData->term[1] = tmpvNode->leafData->term[2];
				tmpvNode->leafData->term[2] = NULL;
			} else if (position == 4) {
				newvNode->leafData->term[0] = newTerm;
				newvNode->leafData->term[1] = tmpvNode->leafData->term[2];
				tmpvNode->leafData->term[2] = NULL;
			} else if (position == 6) {
				newvNode->leafData->term[0] = tmpvNode->leafData->term[2];
				newvNode->leafData->term[1] = newTerm;
				tmpvNode->leafData->term[2] = NULL;
			}
			parent->nodeData->child1 = tmpvNode;
			parent->nodeData->child2 = newvNode;
			parent->nodeData->key1 = newvNode->leafData->term[0]->word;
		}
		/*如果此结点是根结点且是非叶结点，父结点为NULL,直接申请一个新vNode作为root，并新申请一个vNode作为另一个分裂非叶结点*/
		/*接受分裂的两个子vNode地址，将key和地址重新分配，为root分配地址，别忘了parent这一项*/
		else {
			parent = (struct vNode *) malloc(sizeof(struct vNode));
			parent->isLeaf = false;
			parent->leafData = NULL;
			parent->parent = NULL;
			parent->nodeData = (struct node *) malloc(sizeof(struct node));
			parent->nodeData->child1 = NULL; /*需要赋值*/
			parent->nodeData->child2 = NULL; /*需要赋值*/
			parent->nodeData->child3 = NULL;
			parent->nodeData->key1 = NULL; /*需要赋值*/
			parent->nodeData->key2 = NULL;
			tmpvNode->parent = parent;
			Index.root = parent;
			struct vNode * newvNode = (struct vNode *) malloc(
					sizeof(struct vNode));
			newvNode->isLeaf = false;
			newvNode->leafData = NULL;
			newvNode->parent = parent;
			newvNode->nodeData = (struct node *) malloc(sizeof(struct node));
			newvNode->nodeData->child1 = NULL;
			newvNode->nodeData->child2 = NULL;
			newvNode->nodeData->child3 = NULL;
			newvNode->nodeData->key1 = NULL;
			newvNode->nodeData->key2 = NULL;
			int result1 = strcmp(key, tmpvNode->nodeData->key1);
			int result2 = strcmp(key, tmpvNode->nodeData->key2);

			/*如果key字典序小于此结点的key1*/
			if (result1 < 0) {
				newvNode->nodeData->key1 = tmpvNode->nodeData->key2;
				parent->nodeData->key1 = tmpvNode->nodeData->key1;
				tmpvNode->nodeData->key1 = key;
				tmpvNode->nodeData->key2 = NULL;

				parent->nodeData->child1 = tmpvNode;
				parent->nodeData->child2 = newvNode;
				newvNode->nodeData->child1 = tmpvNode->nodeData->child2;
				newvNode->nodeData->child2 = tmpvNode->nodeData->child3;
				tmpvNode->nodeData->child2->parent = newvNode;
				tmpvNode->nodeData->child3->parent = newvNode;
				tmpvNode->nodeData->child1 = child1;
				tmpvNode->nodeData->child2 = child2;
				tmpvNode->nodeData->child3 = NULL;
				child1->parent = tmpvNode;
				child2->parent = tmpvNode;
			}
			/*如果key字典序大于key1且小于key2*/
			else if (result1 > 0 && result2 < 0) {
				newvNode->nodeData->key1 = tmpvNode->nodeData->key2;
				parent->nodeData->key1 = key;
				tmpvNode->nodeData->key2 = NULL;

				parent->nodeData->child1 = tmpvNode;
				parent->nodeData->child2 = newvNode;
				tmpvNode->parent = parent;
				newvNode->parent = parent;
				newvNode->nodeData->child2 = tmpvNode->nodeData->child3;
				tmpvNode->nodeData->child3->parent = newvNode;
				tmpvNode->nodeData->child2 = child1;
				child1->parent = tmpvNode;
				newvNode->nodeData->child1 = child2;
				child2->parent = newvNode;
				tmpvNode->nodeData->child3 = NULL;
			}
			/*如果key字典序大于key2*/
			else {
				newvNode->nodeData->key1 = key;
				parent->nodeData->key1 = tmpvNode->nodeData->key2;
				tmpvNode->nodeData->key2 = NULL;
				parent->nodeData->child1 = tmpvNode;
				parent->nodeData->child2 = newvNode;
				tmpvNode->parent = parent;
				newvNode->parent = parent;
				newvNode->nodeData->child1 = child1;
				child1->parent = newvNode;
				newvNode->nodeData->child2 = child2;
				child2->parent = newvNode;
				tmpvNode->nodeData->child3 = NULL;
			}

		}
	}
	/*要分裂的结点不是根结点*/
	/*要分裂的结点是叶结点，由于是叶结点分裂，需要新建一个term项*/
	/*如果parent已经满了，那么末尾处调用分裂函数；否则，直接处理结束*/
	else if (tmpvNode->isLeaf == true) {
		/*新建叶结点vNode，需要赋值两个term*/
		struct vNode * newvNode = (struct vNode *) malloc(sizeof(struct vNode));
		newvNode->isLeaf = true;
		newvNode->nodeData = NULL;
		newvNode->parent = tmpvNode->parent;
		newvNode->leafData = (struct leaf *) malloc(sizeof(struct leaf));
		newvNode->leafData->term[0] = NULL; /*需要赋值*/
		newvNode->leafData->term[1] = NULL; /*需要赋值*/
		newvNode->leafData->term[2] = NULL;
		struct term * newTerm = (struct term *) malloc(sizeof(struct term));
		newTerm->word = key;
		newTerm->frequency = 1;
		newTerm->articleList = (struct article *) malloc(
				sizeof(struct article));
		newTerm->articleList->frequency = 1;
		newTerm->articleList->ID = articleID;
		newTerm->articleList->next = NULL;
		struct term * tmpTerm;
		if (position == 2) {
			tmpTerm = tmpvNode->leafData->term[1];
			tmpvNode->leafData->term[1] = newTerm;

			newvNode->leafData->term[0] = tmpTerm;
			newvNode->leafData->term[1] = tmpvNode->leafData->term[2];
			tmpvNode->leafData->term[2] = NULL;
		} else if (position == 4) {
			newvNode->leafData->term[0] = newTerm;
			newvNode->leafData->term[1] = tmpvNode->leafData->term[2];
			tmpvNode->leafData->term[2] = NULL;
		} else if (position == 6) {
			newvNode->leafData->term[0] = tmpvNode->leafData->term[2];
			newvNode->leafData->term[1] = newTerm;
			tmpvNode->leafData->term[2] = NULL;
		}
		/*如果父结点满了，再次调用分裂函数*/
		if (tmpvNode->parent->nodeData->key2 != NULL) {
			if (newvNode->leafData->term[0] == NULL) {
				printf(" newvNode->leafData->term[0]->word is NULL\n");
			}

			if (!(tmpvNode->parent != NULL && tmpvNode != NULL
					&& newvNode != NULL))
				printf("NULL\n");
			split(newvNode->leafData->term[0]->word, tmpvNode->parent, tmpvNode,
					newvNode);
		}
		/*父结点未满，直接插入*/
		else {
			int result = strcmp(newvNode->leafData->term[0]->word,
					tmpvNode->parent->nodeData->key1);
			/*key与父结点第一个key比较*/
			if (result < 0) {
				tmpvNode->parent->nodeData->key2 =
						tmpvNode->parent->nodeData->key1;
				tmpvNode->parent->nodeData->key1 =
						newvNode->leafData->term[0]->word;
				tmpvNode->parent->nodeData->child3 =
						tmpvNode->parent->nodeData->child2;
				tmpvNode->parent->nodeData->child1 = tmpvNode;
				tmpvNode->parent->nodeData->child2 = newvNode;
				/*这两个叶结点的父结点已经在之前赋值过了*/
			} else {
				tmpvNode->parent->nodeData->key2 =
						newvNode->leafData->term[0]->word;
				tmpvNode->parent->nodeData->child3 = newvNode;
			}
		}
	}
	/*要分裂的结点既不是根结点也不是叶结点*/
	/*如果parent已经满了，分裂后，key赋值，那么末尾处调用分裂函数；否则，分裂后，key赋值，父结点child赋值，直接处理结束*/
	else {
		struct vNode * newvNode = (struct vNode *) malloc(sizeof(struct vNode));
		newvNode->isLeaf = false;
		newvNode->leafData = NULL;
		newvNode->parent = tmpvNode->parent;
		newvNode->nodeData = (struct node *) malloc(sizeof(struct node));
		newvNode->nodeData->child1 = NULL; /*需要赋值*/
		newvNode->nodeData->child2 = NULL; /*需要赋值*/
		newvNode->nodeData->child3 = NULL;
		newvNode->nodeData->key1 = NULL;
		newvNode->nodeData->key2 = NULL;

		/*父结点未满*/
		if (tmpvNode->parent->nodeData->key2 == NULL) {
			int result1 = strcmp(key, tmpvNode->nodeData->key1);
			if (tmpvNode->nodeData->key2 == NULL) {
				printf("NULL\n");
			}
			int result2 = strcmp(key, tmpvNode->nodeData->key2);
			char * keyUp; /*用于存储上滤的key*/
			/*如果key字典序小于此结点的key1*/
			if (result1 < 0) {
				newvNode->nodeData->key1 = tmpvNode->nodeData->key2;
				keyUp = tmpvNode->nodeData->key1;
				tmpvNode->nodeData->key1 = key;
				tmpvNode->nodeData->key2 = NULL;

				newvNode->nodeData->child1 = tmpvNode->nodeData->child2;
				newvNode->nodeData->child2 = tmpvNode->nodeData->child3;
				tmpvNode->nodeData->child2->parent = newvNode;
				tmpvNode->nodeData->child3->parent = newvNode;
				tmpvNode->nodeData->child1 = child1;
				tmpvNode->nodeData->child2 = child2;
				tmpvNode->nodeData->child3 = NULL;
				child1->parent = tmpvNode;
				child2->parent = tmpvNode;
			}
			/*如果key字典序大于key1且小于key2*/
			else if (result1 > 0 && result2 < 0) {
				newvNode->nodeData->key1 = tmpvNode->nodeData->key2;
				keyUp = key;
				tmpvNode->nodeData->key2 = NULL;

				newvNode->nodeData->child2 = tmpvNode->nodeData->child3;
				tmpvNode->nodeData->child3->parent = newvNode;
				tmpvNode->nodeData->child2 = child1;
				child1->parent = tmpvNode;
				newvNode->nodeData->child1 = child2;
				child2->parent = newvNode;
				tmpvNode->nodeData->child3 = NULL;
			}
			/*如果key字典序大于key2*/
			else {
				newvNode->nodeData->key1 = key;
				keyUp = tmpvNode->nodeData->key2;
				tmpvNode->nodeData->key2 = NULL;

				newvNode->nodeData->child1 = child1;
				child1->parent = newvNode;
				newvNode->nodeData->child2 = child2;
				child2->parent = newvNode;
				tmpvNode->nodeData->child3 = NULL;
			}
			/*确定了keyUp和两个结点地址，处理未满的父结点*/
			/*上滤的keyUp字典序大于父结点的key1*/
			/*确定了keyUp和两个结点地址，处理未满的父结点*/
			/*上滤的keyUp字典序大于父结点的key1*/
			if (strcmp(keyUp, tmpvNode->parent->nodeData->key1) > 0) {
				tmpvNode->parent->nodeData->key2 = keyUp;
				tmpvNode->parent->nodeData->child2 = tmpvNode;
				tmpvNode->parent->nodeData->child3 = newvNode;
				/*父结点未分裂，两个结点的父结点指针无需变化*/
			}
			/*上滤的keyUp字典序小于父结点的key1*/
			else {
				tmpvNode->parent->nodeData->key2 =
						tmpvNode->parent->nodeData->key1;
				tmpvNode->parent->nodeData->key1 = keyUp;
				tmpvNode->parent->nodeData->child3 =
						tmpvNode->parent->nodeData->child2;
				tmpvNode->parent->nodeData->child2 = newvNode;
				tmpvNode->parent->nodeData->child1 = tmpvNode;
			}
		}
		/*父结点满了,父结点需要分裂*/
		else {
			int result1 = strcmp(key, tmpvNode->nodeData->key1);
			int result2 = strcmp(key, tmpvNode->nodeData->key2);
			char * keyUp; /*用于存储上滤的key*/
			if (result1 < 0) {
				newvNode->nodeData->key1 = tmpvNode->nodeData->key2;
				keyUp = tmpvNode->nodeData->key1;
				tmpvNode->nodeData->key1 = key;
				tmpvNode->nodeData->key2 = NULL;

				newvNode->nodeData->child1 = tmpvNode->nodeData->child2;
				newvNode->nodeData->child2 = tmpvNode->nodeData->child3;
				tmpvNode->nodeData->child2->parent = newvNode;
				tmpvNode->nodeData->child3->parent = newvNode;
				tmpvNode->nodeData->child1 = child1;
				tmpvNode->nodeData->child2 = child2;
				tmpvNode->nodeData->child3 = NULL;
				child1->parent = tmpvNode;
				child2->parent = tmpvNode;
			}
			/*如果key字典序大于key1且小于key2*/
			else if (result1 > 0 && result2 < 0) {
				newvNode->nodeData->key1 = tmpvNode->nodeData->key2;
				keyUp = key;
				tmpvNode->nodeData->key2 = NULL;

				newvNode->nodeData->child2 = tmpvNode->nodeData->child3;
				tmpvNode->nodeData->child3->parent = newvNode;
				tmpvNode->nodeData->child2 = child1;
				child1->parent = tmpvNode;
				newvNode->nodeData->child1 = child2;
				child2->parent = newvNode;
				tmpvNode->nodeData->child3 = NULL;
			}
			/*如果key字典序大于key2*/
			else {
				newvNode->nodeData->key1 = key;
				keyUp = tmpvNode->nodeData->key2;
				tmpvNode->nodeData->key2 = NULL;

				newvNode->nodeData->child1 = child1;
				child1->parent = newvNode;
				newvNode->nodeData->child2 = child2;
				child2->parent = newvNode;
				tmpvNode->nodeData->child3 = NULL;
			}
			/*因为父结点要分裂，直接调用函数进行下一次处理*/
			split(keyUp, tmpvNode->parent, tmpvNode, newvNode);
		}
	}
}

void insert(char *key) {
	struct vNode * tmp = NULL;
//	printf("Inserting %s\n", key);
	/*根为空*/
	if (Index.root == NULL) {
		Index.root = (struct vNode *) malloc(sizeof(struct vNode));
		Index.root->isLeaf = true;
		Index.root->parent = NULL;
		Index.root->nodeData = NULL;
		Index.root->leafData = (struct leaf *) malloc(sizeof(struct leaf));
		Index.root->leafData->term[0] = NULL;
		Index.root->leafData->term[1] = NULL;
		Index.root->leafData->term[2] = NULL;
		Index.root->leafData->term[0] = (struct term *) malloc(
				sizeof(struct term));
		Index.root->leafData->term[0]->word = key;
		Index.root->leafData->term[0]->frequency = 1;
		Index.root->leafData->term[0]->articleList = (struct article *) malloc(
				sizeof(struct article));
		Index.root->leafData->term[0]->articleList->ID = articleID;
		Index.root->leafData->term[0]->articleList->frequency = 1;
		Index.root->leafData->term[0]->articleList->next = NULL;
	}
	tmp = find(Index.root, key);
	/*找到对应项*/
	if (position == 1 || position == 3 || position == 5) {
		/*直接对对应term的frequency进行+1，并且判断是否articleList里面是否已经存在与本文章ID相同的项*/
		struct article * tmpArticle;
		struct article * tmpPre;

		tmpArticle = tmp->leafData->term[(position - 1) / 2]->articleList;
		while (tmpArticle != NULL) {
			tmpPre = tmpArticle;
			if (tmpArticle->ID == articleID) {
//				printf("article exist\n");
				tmpArticle->frequency++;
				return;
			} else
				tmpArticle = tmpArticle->next;
		}
		tmp->leafData->term[(position - 1) / 2]->frequency++;
		/*没找到相同ID的项，插入新的article项至链表中*/
//		printf("insert new article\n");
		tmpPre->next = (struct article *) malloc(sizeof(struct article));
		tmpPre->next->frequency = 1;
		tmpPre->next->ID = articleID;
		tmpPre->next->next = NULL;
	}

	/*未找到对应项*/
	/*判断是否此叶结点满了，如果满了，则分裂，分裂后判断父结点是否满，知道此父结点是根结点，则增加一个root；否则，直接插入对应的位置*/

	else if (tmp->leafData->term[1] == NULL) {
//		printf("first term of leaf data is NULL\n");
		tmp->leafData->term[1] = (struct term *) malloc(sizeof(struct term));
		tmp->leafData->term[1]->frequency = 1;
		tmp->leafData->term[1]->articleList = (struct article *) malloc(
				sizeof(struct article));
		tmp->leafData->term[1]->word = key;
		tmp->leafData->term[1]->articleList->frequency = 1;
		tmp->leafData->term[1]->articleList->ID = articleID;
		tmp->leafData->term[1]->articleList->next = NULL;
		if (position != 2) {
			printf("error position should be 2, but it is %d\n", position);
		}
	} else if (tmp->leafData->term[2] == NULL && position == 4) {
//		printf("second term of leaf data is NULL\n");
		tmp->leafData->term[2] = (struct term *) malloc(sizeof(struct term));
		tmp->leafData->term[2]->frequency = 1;
		tmp->leafData->term[2]->articleList = (struct article *) malloc(
				sizeof(struct article));
		tmp->leafData->term[2]->word = key;
		tmp->leafData->term[2]->articleList->frequency = 1;
		tmp->leafData->term[2]->articleList->ID = articleID;
		tmp->leafData->term[2]->articleList->next = NULL;
	} else if (tmp->leafData->term[2] == NULL && position == 2) {
//		printf("second term of leaf data is NULL\n");
		tmp->leafData->term[2] = tmp->leafData->term[1];
		tmp->leafData->term[1] = (struct term *) malloc(sizeof(struct term));
		tmp->leafData->term[1]->frequency = 1;
		tmp->leafData->term[1]->articleList = (struct article *) malloc(
				sizeof(struct article));
		tmp->leafData->term[1]->word = key;
		tmp->leafData->term[1]->articleList->frequency = 1;
		tmp->leafData->term[1]->articleList->ID = articleID;
		tmp->leafData->term[1]->articleList->next = NULL;
	} else if (tmp->leafData->term[2] == NULL && position == 6) {
		printf("error position should be 2 or 4 , but it is %d\n", position);
	}
	/*这个叶结点已经满了，需要分裂*/
	else {
//		printf("new word\n");
		split(key, tmp, NULL, NULL);
	}
}

int findnoise(char * buf){
	int i=0;
	while(i<100){
		if(strcmp(noise[i], buf)==0)return 1;
		i++;
	}
	return 0;
}

/*读取文章并建造index*/
void read(char * abuf, int alength, FILE *fpa) {
	char* buf;
	buf = (char *) malloc(40 * sizeof(char)); /*假设每个单词最多39位*/
	memset(buf, 0, 40);
	char ch;
	int length;
	fscanf(fpa, "%s", buf);
	while (!feof(fpa)) {
		ch = buf[strlen(buf) - 1];
		if (ch == ',' || ch == '.' || ch == ';' || ch == '\'') {
			buf[strlen(buf) - 1] = 0; /*去掉在单词末尾的四种符号*/
		}
		length = strlen(buf);
//		printf("inserting: %s\n", buf);
		if (!findnoise(buf)) {
			insert(buf);
		}
//		printf("word tital: %d\n", debugcount);
		buf = (char *) malloc(40 * sizeof(char));
		fscanf(fpa, "%s", buf);
	}
	return;
}


void noiseIndex() {
	FILE * nf_in;
	if (NULL == (nf_in = fopen("noise words/noise_500.txt", "r"))) {
		printf("error open noise_500.txt\n");
		exit(1);
	} /*打开对应的文章*/
	char* buf;
	buf = (char *) malloc(40 * sizeof(char)); /*假设每个单词最多39位*/
	memset(buf, 0, 40);
	char ch;
	int i = 0;
	fscanf(nf_in, "%s", buf);
	while ((!feof(nf_in)) && i < 100) {
		noise[i] = buf;
		i++;
		buf = (char *) malloc(40 * sizeof(char));
		fscanf(nf_in, "%s", buf);
	}
	fclose(nf_in);
	printf("noise index built successfully!\n");
	return;

}
int main() {
	noiseIndex();

//	while(1){;}
	char out[120];
	strcpy(out, "temp.txt");
	FILE * f_in;
	FILE * f_out;
	struct sb_stemmer * stemmer;
	clock_t startIndex, endIndex;

	char language[120];
	strcpy(language, "english");
	char * charenc = NULL;
	stemmer = sb_stemmer_new(language, charenc);
	pretty = 0;

	Index.isEstablished = false;
	Index.root = NULL;
	char * buf;
	buf = NULL;
	char minstring[10];
	strcpy(minstring, "\0");

	buf = (char *) malloc(40 * sizeof(char));
	/*建立一个用于搜索的index项*/
	FILE *fp;
	FILE * fpa;
	char path[120];
	memset(path, 0, 120);
	strcpy(path, "shakespeareWhole/");
	strcat(path, "index.txt");
	if (NULL == (fp = fopen(path, "r"))) {
		printf("error open index\n");
		exit(1);
	}
	insert(minstring);
	startIndex = clock();
	while (fgets(buf, 39, fp) != NULL) /*读完所有的文章*/
	{
		articleID++;
		buf[strlen(buf) - 1] = '\0'; /*去掉末尾的换行符'\n',此时文章标题的length减1*/
		printf("Indexing file: %s\n", buf);
		memset(path, 0, 120);
		strcpy(path, "shakespeareWhole/");
		strcat(path, buf);
		if (NULL == (f_in = fopen(path, "r"))) {
			printf("error open book\n");
			exit(1);
		} /*打开对应的文章*/
		f_out = fopen(out, "w");
		if (f_out == 0) {
			fprintf(stderr, "file %s cannot be opened\n", out);
			exit(1);
		}
		/* do the stemming process: */
		stem_file(stemmer, f_in, f_out);
		fclose(f_out);
		fclose(f_in);
		if (NULL == (fpa = fopen(out, "r"))) {
			printf("error open book\n");
			exit(1);
		} /*open corresponding file after stemming*/
		read(buf, strlen(buf), fpa);
		fclose(fpa);
	}
	fclose(fp);
	endIndex = clock();
	printf("Indexing time=%f\n\n", (double) (endIndex - startIndex));
	printf("index built successfully!\n");
	/*建立完毕，可以写搜索的函数用于检测是否有bug*/

	/*do 3 searches*/
	struct vNode * tmp;
	struct article * tmpArticle;
	clock_t start, end;

	char qword[40];
	memset(qword, 0, 40);
	char sqword[10];
	memset(sqword, 0, 40);
	int i, j;
	int Max = 10;/*max article number to show*/
	for (i = 0; i < 30; i++) {
		j = 0;
		printf("Query word:");
		scanf("%s", sqword);
		f_in = fopen("que.txt", "w");
		fprintf(f_in, "%s\n", sqword);
		fclose(f_in);
		f_in = fopen("que.txt", "r");
		f_out = fopen(out, "w");
		stem_file(stemmer, f_in, f_out);
		fclose(f_out);
		fclose(f_in);
		if (NULL == (fpa = fopen(out, "r"))) {
			printf("error open book\n");
			exit(1);
		} /*open corresponding file after stemming*/
		fscanf(fpa, "%s", qword);
		printf("after stemming: %s\n\n", qword);
		fclose(fpa);
		start = clock();
		tmp = find(Index.root, qword);
		end = clock();
		if (position == 1 || position == 3 || position == 5) {
			printf("time=%f\n\n", (double) (end - start));
			printf("word found. and article number is %d\n",
					tmp->leafData->term[(position - 1) / 2]->frequency);
			tmpArticle = tmp->leafData->term[(position - 1) / 2]->articleList;
			while (j < Max) {
				printf("ID: %d\n", tmpArticle->ID);
				printf("Frequency: %d\n\n", tmpArticle->frequency);
				tmpArticle = tmpArticle->next;
				if (tmpArticle == NULL)
					break;
				j++;
			}
		} else {
			printf("word not found.\n");
		}
	}

	return 0;

}
