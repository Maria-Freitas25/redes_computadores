#ifndef SPLIT_H_
#define SPLIT_H_

#define MAX_PARTS 255

/**
 * Define a estrutura da lista de palavras obtida como valor de retorno
 * da função de divisão.
 */
typedef struct {
    char * parts[MAX_PARTS];
    int count;
}split_list;

/** 
 * Divide uma string em palavras, usando os delimitadores especificados
 * ou os delimitadores padrão
 */
split_list * split(char * str, const char * delim);

#endif
