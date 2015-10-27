#include "nemu.h"
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include<string.h>

enum {
	NOTYPE = 256, EQ , DEC  , HEX , REG , NEG , LE , GE=263 , 
	DREF = 264 , SL , SR , NEQ , AND , OR
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */
	//rules and type(in the enum above)
	{"0x[0-9a-fA-F]+" , HEX} ,		//hexadecimal number
	{"[0-9]+" , DEC} ,				//decimal number
	{" +",	NOTYPE},				// spaces
	{"\\+", '+'},					// plus
	{"<=" , LE} , 
	{">=" , GE} , 
	{">" , '>'} ,
	{"<" , '<'} , 
	{"==", EQ} , 					// equal
									//leaving NEG for future parsing
	{"-" , '-'} ,					//minus
	{"\\*" , '*'} ,				    //multiply
	{"/" , '/'} ,			    	//devide
	{"\\$[a-zA-Z]{2,3}" , REG} ,    //register TODO:consider printing "wrong register"
	{"\\\(" , '('} , {"\\)" , ')'} ,//parenthesis
	{"%" , '%'}						//mod
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )
 regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */
				
				//expression overflow
				if(nr_token>=31||substr_len>31){
					printf("expression too long\n"); 
					return false; 
				}
				switch(rules[i].token_type) {
					
				default:
					//plain recording
					//nr_token is the number of tokens (1-32)
					tokens[nr_token].type = rules[i].token_type;
					strncpy(tokens[nr_token].str , substr_start , substr_len); 
					(tokens[nr_token].str)[substr_len]='\0';
					nr_token++;
					//TODO: panic("please implement me");
						 
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}


bool check_parentheses(int p , int q){
	//pretend there is a stack always full of '('
	int end=-1; 
	if(tokens[p].type!='('||tokens[q].type!=')')
		return false; 
	int i;
	for(i=p; i<=q; i++){
		if(tokens[i].type=='('){
			++end;
			continue; 
		}
		if(tokens[i].type==')'){
			if(end==-1)return false; 
			if(end==0 && i!=q)return false; 
			end--; 
			continue;  
		}
	}
	if(end!=-1)return false;
	return true;
}

int get_operator_priority(int operator){
	switch(operator){
		case NEG:case DREF:case '!':case '~':
			return 18; 
		case '/':case '*':case '%':
			return 17; 
		case '+':case '-':
			return 16; 
		case SL:case SR:
			return 15; 
		case '>':case GE:case '<':case LE:
			return 14; 
		case EQ:case NEQ:
			return 13; 
		case '&':return 12; 
		case '^':return 11; 
		case '|':return 10; 
		case AND:return 9; 
		case OR:return 8; 
		default: return 888; 
	}
}
long long string_to_int(char *s , int base){
	int i , n=strlen(s); 
	if(!s)return 0; 
	long long ans=0; 
	for(i=0; i<n; i++)
		ans = ans*base + s[i] - '0'; 
	return ans; 
}

//TODO
#define MZYDEBUG
long long eval(int p , int q){
	//p , q is the beginning and ending of a subexpression
	if(p>q){
		/*bad expression*/
		panic("p > q in eval()"); 
		return 0; 
	}
	else if(p == q) { 
		/* Single token.
		 *		 * For now this token should be a number. 
		 *				 * Return the value of the number.
		 *						 */ 
		if(tokens[p].type==DEC)
			return string_to_int(tokens[p].str , 10);  
		if(tokens[p].type==HEX)
			return string_to_int(tokens[p].str , 16); 
	}
	else if(check_parentheses(p ,  q) == true) {
		/* The expression is surrounded by a matched pair of parentheses. 
		 *		 * If that is the case ,  just throw away the parentheses.
		 *				 */
		return eval(p + 1 ,  q - 1);  
	}
	else {
		//dominant operator
		int i , op=p , op_priority = get_operator_priority(tokens[p].type); 
		//op is the position of current choice of dominant operator
		for(i=p; i<=q; i++){
			if(tokens[i].type=='('){
				int count=1; 
				while(++i<=q && count){
					if(tokens[i].type==')')
						count--; 
					else if(tokens[i].type=='(')
						count++; 
				}
				if(!count)continue;
#ifdef MZYDEBUG
				else panic("eval() exception after hitting '(' when searching for dominant operators\n"); 
#endif
			}
			//now i is not in a pair of paren
			else if(tokens[i].type==DEC || tokens[i].type==HEX)continue; 
			else if(get_operator_priority(tokens[i].type) <= op_priority ){
				op_priority = get_operator_priority(tokens[i].type); 
				op = i; 
			}
		}
		//now op is the dominant operator
		long long val1 = eval(p , op-1); 
		long long val2 = eval(op+1 , q);
#ifdef MZYDEBUG
		printf("p=%d , q=%d , val1=%lld, op=%d , val2=%lld\n " ,p , q ,  val1 , op , val2); 
#endif
		switch(tokens[op].type){
			case '+':return val1+val2; 
			case '-':return val1-val2; 
			case '*':return val1*val2; 
			case '/':return (double)val1/val2; 
			default:printf( "operator %c not defined.\n" , tokens[op].type); 
			return 0; 
		}


	

		/* We should do more things here. */
	}
	return 0; 

}

uint32_t expr(char *e, bool *success) {
	//if(!strlen(e))return 0; 
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	int i; for(i=0; i<nr_token; i++)printf("%c " , tokens[i].type); 
#ifdef MZYDEBUG
	int paren=check_parentheses(0 , nr_token-1)?1:0;  
	printf("paren %d\n" , paren); 
	printf("-----end of tokening-----\n" );
	if(!paren){
		printf("invalid expression.\n"); 
		return 0; 
	}
#endif
	/* TODO: Insert codes to evaluate the expression. */
	//TODO:panic("please implement me");
	static int gdb_expr_count=0; 
	long long ans = eval(0 , nr_token-1);
	printf(" $%d\t\t%lld\n" , gdb_expr_count , ans); 
	return 0;
}

