//

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
#include <ctype.h>  /* for isupper, tolower */
#include <time.h>

#include "./libstemmer_c/include/libstemmer.h"
const char * progname;
static int pretty = 1;

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

struct term {
	char * word; /*store the word in string*/
//	struct article * articleList; /*store articles that contain the word*/
	int frequency; /*store the times of a specific word*/
	struct term * next;
};

struct index {
	bool isEstablished;
	struct term * termList;
};
struct index Index;
//char forDebug;

/*implementation with linked list*/
/*abuf denotes article's ID*/
/*parameter alength is unused in this implementation*/
/*buf denotes word to be stored*/
/*parameter length is unused in this implementation*/
void build(char * buf) {
	if (Index.termList == NULL) {
		/*if index is empty*/

		/*create first(empty) node for a word list*/
		Index.termList = (struct term *) malloc(sizeof(struct term));
		Index.termList->word = NULL;
		Index.termList->frequency = -1;
//		Index.termList->articleList = NULL;

		/*create first(real) node for a word list*/
		Index.termList->next = (struct term *) malloc(sizeof(struct term));
		Index.termList->next->word = buf;
		Index.termList->next->frequency = 1;
		Index.termList->next->next = NULL;

//		/*create first(empty) node for an article list*/
//		Index.termList->next->articleList = (struct article *) malloc(
//				sizeof(struct article));
//		Index.termList->next->articleList->name = NULL;
//		Index.termList->next->articleList->frequency = -1;
//		Index.termList->next->articleList->next = NULL;

//		/*create first(real) node for an article list*/
//		Index.termList->next->articleList->next = (struct article *) malloc(
//				sizeof(struct article));
//		Index.termList->next->articleList->next->frequency = 1;
//		Index.termList->next->articleList->next->name = abuf;
//		Index.termList->next->articleList->next->next = NULL;

		Index.isEstablished = true;
	} else {
		/*3 cases: 1. find word->find article; 2. find word->can't find article; 3. can't find word*/
		struct term *tmpTerm;
		tmpTerm = Index.termList->next;
		while (tmpTerm != NULL) {
			/*traverse, compare every words with word for indexing*/
			int result;
			result = strcmp(buf, tmpTerm->word);
			if (result != 0) /*word doesn't match, continue*/
			{
				tmpTerm = tmpTerm->next;
				continue;
			} else if (result == 0) /*match the word*/
			{
				tmpTerm->frequency++;
//				if(!strcmp(buf, "Romeo")){printf("Romeo++\n");}
				return;
			}
		}
		/*no match for this word, which means the word is new*/

		/*insert a new word node*/
		//printf("adding new word: %s\n", buf);
		tmpTerm = Index.termList->next;
		Index.termList->next = (struct term *) malloc(sizeof(struct term));
		Index.termList->next->next = tmpTerm; /*link new word to the rest index*/
		Index.termList->next->word = buf;
		Index.termList->next->frequency = 1;
		return;

	}
	return;
}

/*read file and build Index*/
void read(FILE *fpa) {
	char* buf;
	buf = (char *) malloc(40 * sizeof(char)); /*suppose all words have max length 39*/
	memset(buf, 0, 40);
	char ch;
	int length;
	fscanf(fpa, "%s", buf);
	while (!feof(fpa)) {
		ch = buf[strlen(buf) - 1];
		if (ch == ',' || ch == '.' || ch == ';' || ch == '\'') {
			buf[strlen(buf) - 1] = 0; /*get rid of 4 kinds of common punctuation*/
		}
//		length = strlen(buf);
		build(buf);
		buf = (char *) malloc(40 * sizeof(char));
		memset(buf, 0, 40);
		fscanf(fpa, "%s", buf);
	}
//	abuf = (char *) malloc(40 * sizeof(char));
	return;
}

int main() {
	//char * in = "in.txt";
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
	Index.termList = NULL;
	char * buf;
	buf = NULL;
	buf = (char *) malloc(40 * sizeof(char));
	/*build the Index for searching*/
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
	//printf("Opening index\n");
	startIndex = clock();
	while (fgets(buf, 39, fp) != NULL) /*read all files*/
	{
		buf[strlen(buf) - 1] = '\0'; /*delete '\n'*/
		printf("Indexing file: %s\n", buf);
		memset(path, 0, 120);
		strcpy(path, "shakespeareWhole/");
		strcat(path, buf);
		//printf("Path is: %s", path);
		if (NULL == (f_in = fopen(path, "r"))) {
			printf("error open book\n");
			exit(1);
		} /*open corresponding file*/
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
		read(fpa);
		fclose(fpa);
	}

	fclose(fp);
	endIndex = clock();
	printf("time=%f\n\n", (double) (endIndex - startIndex));
	printf("index built successfully!\n");

	/*index built*/

	/*deniose*/
	struct term *tmpTerm;
	struct term *MaxTerm;
	struct term *PMaxTerm;/*the term before Maxterm*/
	int t;
	FILE * f_in2;
	tmpTerm = Index.termList->next;
	MaxTerm = Index.termList->next;
	f_in = fopen("noise_frequency_500.txt", "w");
	f_in2 = fopen("noise_500.txt", "w");
	for(t=0;t<500;t++){
		tmpTerm = Index.termList;
		while(tmpTerm!=NULL){
			if((tmpTerm->next!=NULL)&&(tmpTerm->next->frequency > MaxTerm->frequency)){
				MaxTerm = tmpTerm->next;
				PMaxTerm = tmpTerm;
			}
			tmpTerm = tmpTerm->next;
		}
		fprintf(f_in, "%s\t\t%d\n", MaxTerm->word, MaxTerm->frequency);
		fprintf(f_in2, "%s\n", MaxTerm->word);
		PMaxTerm->next = MaxTerm->next;
		free(MaxTerm);
		MaxTerm = Index.termList->next;
	}
	printf("Noise over!\n");

	fclose(f_in);
	fclose(f_in2);
	sb_stemmer_delete(stemmer);
	return 0;
}
