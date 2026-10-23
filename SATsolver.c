#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

//---------------structs--------------

typedef enum
{
    VERDADEIRO = 1,
    FALSO = 0,
    INDEFINIDO = 2,
}valor;

typedef struct node
{
    int variavel;
    valor estado;
    struct node *left;
    struct node *right;
}node;

typedef struct
{
    int num_literais;
    int* literais;
}clausula;

typedef struct
{
    int num_variaveis;
    int num_clausulas;
    clausula* clausulas;
}formula;

typedef struct
{
    valor* valores;
    int num_variaveis;
}intp_parcial;

//--------------- Funcões p/ avaliação da interpretação parcial 

valor avaliar_literal(int literal , intp_parcial *interpretacao)
{
    int variavel = literal;
    if(variavel < 0)
        variavel *= -1;

    valor valor_var = interpretacao->valores[variavel];

    if(valor_var == INDEFINIDO)
        return INDEFINIDO;
    
    if(literal > 0)             //aqui o literal é positivo, logo a variavel vai ser igual ao valor do literal
        return valor_var; 
    
    //se entrou aqui, o literal era negativo, logo a variavel tem valor contrario ao do literal
    if(valor_var == VERDADEIRO)
        return FALSO;
    return VERDADEIRO; //como já foi tratado a variavel sendo indefinida, so resta o falso e verdadeiro, e tratando o f so sobrou o v, então não precisa de mais uma comparação
}

valor avaliar_clausula(clausula *clas, intp_parcial *interpretacao)
{
    bool verifica_ind = false;
    
    for(int i = 0 ; i < clas->num_literais; i++)
    {
        valor resultado = avaliar_literal(clas->literais[i], interpretacao); 
        
        if(resultado == VERDADEIRO)
            return VERDADEIRO;
        else if(resultado == INDEFINIDO)
            verifica_ind = true;     
    }
    if(verifica_ind)
        return INDEFINIDO;
    return FALSO;
}

valor avaliar_formula(formula *form , intp_parcial *interpretacao)
{
    bool verifica_ind = false;

    for(int i = 0; i < form->num_clausulas  ; i++)
    {
        valor resultado = avaliar_clausula(&form->clausulas[i] , interpretacao);
        
        if(resultado == FALSO)
            return FALSO;
        else if(resultado == INDEFINIDO)
            verifica_ind = true;
    }
    if(verifica_ind)
        return INDEFINIDO;
    return VERDADEIRO;
}

//-------------------funções envolvendo memoria
node* criar_no()
{
    node* new_node = (node *) malloc(sizeof(node));

    new_node->variavel = 0;
    new_node->estado = INDEFINIDO;
    new_node->left = NULL ; 
    new_node->right = NULL; 

    return new_node;
}

intp_parcial *criar_interpretacao(int n)
{
    intp_parcial *interpretacao =(intp_parcial *) malloc(sizeof(intp_parcial));
    
    interpretacao->num_variaveis = n;
    interpretacao->valores = malloc((n+1) * sizeof(valor));

    for(int i = 1; i < n + 1; i++)
        interpretacao->valores[i] = INDEFINIDO;

    return interpretacao;
}   

void free_interpretacao(intp_parcial *interpretacao)
{
    if(interpretacao == NULL)
        return;
    
    free(interpretacao->valores);
    free(interpretacao);
}

void free_formula(formula *form)
{
    if(form == NULL)
        return;

    for(int i = 0; i < form->num_clausulas;i++)
    {
        free(form->clausulas[i].literais);
    }

    free(form->clausulas);
    free(form);
}

void free_arvore(node *raiz)
{
    if(raiz==NULL)
        return;
    
    free_arvore(raiz->left);
    free_arvore(raiz->right);
    free(raiz);
}


//-=-=-==-=-=-=-=-=-=-=-===-=rec SAT

//--função para sekecionar a variavel
int selecionar_variavel(intp_parcial *interpretacao)
{
    for(int i = 1; i < interpretacao->num_variaveis + 1; i++)
    {
        if(interpretacao->valores[i] == INDEFINIDO)
            return i;
    }
    return 0; //n tem mais xi livre
}

bool SAT(formula *form , intp_parcial *interpretacao, node* no_atual)
{   
    valor result = avaliar_formula(form , interpretacao); 
    no_atual->estado = result;
        
    if(result == VERDADEIRO)
        return true;
    
    if(result == FALSO)
        return false;
    
    int var = selecionar_variavel(interpretacao);
    no_atual->variavel = var;

    interpretacao->valores[var] = VERDADEIRO;
    no_atual->left =  criar_no();
    if(SAT(form , interpretacao, no_atual->left))
        return true;
    
    interpretacao->valores[var] = FALSO;
    no_atual->right = criar_no();
    if(SAT(form , interpretacao, no_atual->right))
        return true;
    
    interpretacao->valores[var] = INDEFINIDO; //as duas falharam, desfaço a atribuição e volto
    return false;
}

//--------------IMPRIMIR arvore 
void imprimir_arvore(node *raiz, int profundidade, char aresta)
{
    if(raiz == NULL)
        return;
 
    for(int i = 0; i < profundidade; i++)
        printf("    ");
 
    if(aresta != ' ')
        printf("%c: ", aresta);
 
    if(raiz->estado == FALSO)
        printf("[X] formula falsa (poda)\n");
    else if(raiz->estado == VERDADEIRO)
        printf("[OK] formula verdadeira\n");
    else
        printf("x%d\n", raiz->variavel);
 
    imprimir_arvore(raiz->left, profundidade + 1, 'T');
    imprimir_arvore(raiz->right, profundidade + 1, 'F');
 
    /* no interno sem filho direito: o ramo "false" nunca foi visitado */
    if(raiz->estado == INDEFINIDO && raiz->right == NULL)
    {
        for(int i = 0; i < profundidade + 1; i++)
            printf("    ");
        printf("F: (nao visitado)\n");
    }
}


// ------------ler no formato dimacs
formula *ler_dimacs(char *nome_arquivo)
{
    //abrir o arquivo
    FILE *arq = fopen(nome_arquivo, "r");
    if(arq == NULL)
    {
        printf("Erro: nao foi possivel abrir '%s'.\n", nome_arquivo);
        return NULL;
    }

    char letra;
    while(fscanf(arq, " %c", &letra) == 1 && letra == 'c')
        fscanf(arq, "%*[^\n]"); //descarta o resto da linha
    
    int v , c;
    if(letra != 'p' || fscanf(arq , " cnf %d %d", &v ,&c) != 2 || v < 0 || c < 0)
    {
        printf("Erro: cabecalho 'p cnf V C' invalido ou não encontrado.\n");
        fclose(arq);
        return NULL;
    }

    //preparar o estado
    formula *form = (formula *) malloc(sizeof(formula));
    form->num_variaveis = v;
    form->num_clausulas = c;
    form->clausulas = (clausula *) calloc(c, sizeof(clausula)); // zerado: num_literais = 0 e literais = null
    
    //lê exatamente c clausulas; cada um terminando em 0
    for(int i = 0; i < c; i++)
    {
        clausula *cl = &form->clausulas[i];
        int lit = -1;   //so precisa ser diferente de 0
        
        while(fscanf(arq, "%d", &lit) == 1 && lit != 0 && abs(lit) <= v)
        {
            cl->literais = (int *) realloc(cl->literais, (cl->num_literais + 1) * sizeof(int));
            cl->literais[cl->num_literais] = lit;
            cl->num_literais++;
        }

        if(lit != 0) //laço parou sem ler o zero que fecha a clausula
        {
            printf("Erro, clausula invalida\n");
            fclose(arq);
            free_formula(form);
            return NULL;
        }

    }
    
    fclose(arq);
    return form;
}
 
//----------função main
int main(int argc, char *argv[])//argv aponta para um vetor de ponteiros, e cada um deles aponta para um texto. Por isso argv[1] é um char * (uma string) 
{
    if(argc < 2)
    {
        printf("Uso: %s arquivo.cnf [-a]\n", argv[0]);
        return 1;
    }
 
    formula *form = ler_dimacs(argv[1]);
    if(form == NULL)
        return 1;
 
    intp_parcial *intp = criar_interpretacao(form->num_variaveis);
    node *raiz = criar_no();
 
    if(SAT(form , intp, raiz))
    {
        printf("SAT!\n");
        for(int i = 1; i < form->num_variaveis + 1 ; i++)
        {
            printf("%d = ", i);
            if(intp->valores[i] == INDEFINIDO)
                printf("indefinido");
            else
                printf("%d", intp->valores[i]);
            if(i != form->num_variaveis)
                printf(", ");
            else
                printf(".\n");
        }
    }
    else
        printf("UNSAT!\n");

    
    printf("\nArvore de busca:\n");
    imprimir_arvore(raiz, 0, ' ');
    
 
    free_arvore(raiz);
    free_interpretacao(intp);
    free_formula(form);
 
    return 0;
} 
