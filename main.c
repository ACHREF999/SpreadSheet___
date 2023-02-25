
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#define SV_IMPLEMENTATION // since all the implementation is imported if the SV_IMPLEMENTATION exists "ifdef" 
						 // then in order to get access to the functionality i need to define it 
						// before i import the library i also like this coding style

#include "./sv.h"
// we need to know the type of cell content

typedef struct Expr Expr;
typedef size_t Expr_Index;
typedef enum {
	BOP_ADD=0,
	BOP_SUB,
	BOP_MUL,
	BOP_DIV,
}Bop_Kind;
typedef enum{
	EXPR_KIND_NUMBER = 0,
	EXPR_KIND_CELL,
	EXPR_KIND_BOP,
}Expr_Kind;


typedef struct{
	Bop_Kind kind;
	Expr_Index lhs;
	Expr_Index rhs; // can be another addition expr
}Expr_Bop;


typedef struct{
	size_t row;
	size_t col;
	
}Cell_Index; // if the contetnt of expr is ex C12 we split it into (C,12) col and row


typedef union{

	double number;
	Cell_Index cell;
	Expr_Bop bop;
	
}Expr_As;


struct Expr{
	Expr_Kind kind;
	Expr_As as;
};


// As you know our program during parsing it produces a tree for the expressions
// However the tree traversal would be recursive and we would have a used a lot of ressources
// TIME and SPACE
// So one solution is to implement a `Stretch Buffer`
// aka dynamic size array
// we dont use pointers anymore but rather indices(which are still considerd pointers [relative ones] );
// 

// this is a wrapper for the expressions array

typedef struct Expr_Buffer{

	size_t count;
	size_t capacity;

	Expr *items; // this is the array of the items

}Expr_Buffer;

Expr_Index expr_buffer_alloc(Expr_Buffer *eb){
	if(eb->count >= eb->capacity){
		if (eb->capacity==0){
			
			assert(eb->items == NULL); // we only want the capacity to be 0 at the creation

			eb->capacity = 64;
		}else{
		
		eb->capacity *=2; // exponential size allocation 
		
		}
	eb->items = realloc(eb->items,sizeof(Expr)* eb->capacity);			
	}

	memset(&eb->items[eb->count],0,sizeof(Expr));
	return eb->count++;
}

Expr *expr_buffer_at(Expr_Buffer *eb,Expr_Index index){
	// obviously we HAVE to restrict the allocation of exprs
	assert(index < eb->count);

	return &eb->items[index];
	// again return the pointer and not the struct so we 
}

typedef enum{
	DIR_LEFT=0,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN,
}Dir;

typedef enum{
	CELL_KIND_TEXT = 0,
	CELL_KIND_NUMBER,
	CELL_KIND_EXPR,
	CELL_KIND_CLONE,
}Cell_Kind;

// well recall a union is basically a vraiable can host many types ( we allocate the biggest size of the types
// we might use and treat it as any other pointer in the world ) ;D  

typedef enum{

	UNEVALUATED=0,
	INPROGRESS,
	EVALUATED,

}Eval_Status;

typedef struct Cell_Expr {
	Expr_Index index;
	// bool is_evaluated;
//	Eval_Status status;
	double value;
}Cell_Expr;
typedef union{
	String_View text;
	double number;
	Cell_Expr expr; // SO When we evaluate the expression we have two options : 
	// Either we change the kind right after the evaluation and loose the information about the expression that was in that cell OR we make another struct where we keep track of expr ptr , evaluated or not , and the value of evaluation
	// more space in the Ram BUT we still have information about the expr
	Dir clone;// THIS IS DIRECTION WHAT TO CLONE
	// IF IM CLONE THEN AS.CLONE IS THE DIRECTION TO WHERE IM CLONNING ; WHICH IS THE ONLY INFORMATION WE NEED ABOUT THE CLONE OPERATION
} Cell_As;


// i think it is good practice to call unions NAME_As for domain specific language


// 0 initialising a struct is still a valid struct
typedef struct{
	Cell_Kind kind;
	Eval_Status status;
	Cell_As as;
	size_t file_row;
	size_t file_col;
	
}Cell;

typedef struct {
	Cell *cells;
	size_t rows;
	size_t cols;
	const char *file_path;
}Table;


bool is_name(char c){

	return isalnum(c)|| c=='_';
	
}

// int isdigit(int) different signature so we need to implement a worarround but is duper simple
bool is_digit(char c){
	return isdigit(c);
}



typedef struct {
	String_View src;
	const char *file_path;
	size_t file_row;
	const char *line_start;
}Lexer_Input;

void print_location_lexer(FILE* stream,Lexer_Input *lexer){
	fprintf(stream,"%s : %zu : %zu : ",lexer->file_path,lexer->file_row,lexer->src.data - lexer->line_start +1);
}

String_View next_token(Lexer_Input *lexer){
	lexer->src = sv_trim(lexer->src);

	if(lexer->src.count == 0){
		return SV_NULL;
	}
	// if(*src->data=='='){
	// 	return sv_chop_left(src,1);// we take 1 char
	// }
	if (*lexer->src.data=='+' || *lexer->src.data=='*' || *lexer->src.data=='-' || *lexer->src.data=='/'){
		return sv_chop_left(&lexer->src,1); // take one character out from the left and return it (the chopped char)
	}
//	if(is_digit(*src->data)){
//		return sv_chop_left_while(src,is_digit); // takes a predicate ( func pointer )
//	}
	if (is_name(*lexer->src.data)){
		return sv_chop_left_while(&lexer->src,is_name);
	}
	print_location_lexer(stderr,lexer);
	fprintf(stderr,"ERROR: unknown token starts with %c",
	*lexer->src.data);
	exit(1);
}

typedef struct {
	size_t capacity;
	char *cstr;
}Tmp_Cstr;		//	THE NAMING 

char *tmp_cstr_fill(Tmp_Cstr *t,const char *data,size_t size){
	if(size +1 > t->capacity){
		t->capacity = size +1;
		t->cstr = realloc(t->cstr,t->capacity);
	}

	// now we are sure that the bufffer has enough space to contain the string and the null char
	memcpy(t->cstr,data,size);//where to ; from where , how much
	t->cstr[size] = '\0'; // we add the null char
	return t->cstr;

}

bool sv_strtol(String_View sv,Tmp_Cstr *tc ,long int *out_result ){
	//static char tmp_buffer[1024*4];
	//assert(sv.count<sizeof(tmp_buffer));
	//snprintf(tmp_buffer,sizeof(tmp_buffer),SV_Fmt,SV_Arg(sv));
	
	char *ptr = tmp_cstr_fill(tc,sv.data,sv.count);// ptr>ptr>string

	char *endptr = NULL;
	long int result = strtol(ptr,&endptr,10); // base 10 // strtol && strtod takes 2nd pos arg as ptr ptr char
	if(out_result) *out_result = result;
	return (endptr!=ptr && *endptr == '\0');
}


bool sv_strtod(String_View sv ,Tmp_Cstr *tc, double *out_result){
	//static char tmp_buffer[1024*4];
	//assert(sv.count < sizeof(tmp_buffer));
	//snprintf(tmp_buffer,sizeof(tmp_buffer),SV_Fmt,SV_Arg(sv)) ; // to ensure that the string is null terminated
	char *ptr = tmp_cstr_fill(tc,sv.data,sv.count);
	char *endptr;

	double result = strtod(ptr,&endptr);
// 	We can make it optional !!!
	if (out_result) *out_result = result;

	return (endptr != ptr && *endptr =='\0') ;// we successfully copied and ended the str with \0
		

}


Expr_Index parse_primary_expr(Lexer_Input *lexer,Tmp_Cstr *tc,Expr_Buffer *eb){
	String_View token = next_token(lexer);

	if (token.count ==0){
		print_location_lexer(stderr,lexer);
		fprintf(stderr,"ERROR : expected primary expression token , but got end of input \n");
		
		exit(1);
	
	}
//	if(is_digit(*token->data))

	Expr_Index expr_i = expr_buffer_alloc(eb);
	Expr *expr = expr_buffer_at(eb,expr_i);
	memset(expr,0,sizeof(Expr));
	
//	static char tmp_buffer[1024*4];
//	assert(token.count<sizeof(tmp_buffer));
//	snprintf(tmp_buffer,sizeof(tmp_buffer),SV_Fmt,SV_Arg(token));


//	char *endptr;
//	expr->as.number = strtod(tmp_buffer,&endptr); // we try to convert


//	if(endptr!=tmp_buffer && *endptr=='\0'){
//		expr->kind = EXPR_KIND_NUMBER;
//	}
	if(sv_strtod(token,tc,&expr->as.number)){
		expr->kind = EXPR_KIND_NUMBER;
//		return expr;
	}

	else{
		expr->kind = EXPR_KIND_CELL;
		if(!isupper(*token.data)){
			print_location_lexer(stderr,lexer);
			fprintf(stderr,"ERROR : cell reference gone wrong\n");
			exit(1);
		}
		 // cell has row and col
		expr->as.cell.col = *token.data -'A';
		sv_chop_left(&token,1);// we only have 26 cols
		
		long int row = 0;
		if(!sv_strtol(token,tc,&row)){
			print_location_lexer(stderr,lexer);
			fprintf(stderr,"ERROR: cell ref must have row index");
			exit(1);
		} // after the first char we get the row char => col and num => row

		expr->as.cell.row =(size_t) row; // we don't -1 
		// why ? well we ignore the first row we dont put it into consideration -- 4sumReason didnt see it early `enough`

//		return expr;
		
//		assert(0 && "not implemented");
//		exit(1);
//		expr->kind = EXPR_KIND_TEXT;
//		expr->as.text = token;
	}

	return expr_i;


}



Expr_Index parse_bop_expr(Lexer_Input *lexer,Tmp_Cstr *tc,Expr_Buffer *eb){
	Expr_Index lhs_i = parse_primary_expr(lexer,tc,eb);

	String_View token = next_token(lexer);
	if(token.data != NULL && (sv_eq(token,SV("+")) ||sv_eq(token,SV("*")) ||sv_eq(token,SV("/")) ||sv_eq(token,SV("-"))  )){
		
		
		Expr_Index rhs_i = parse_bop_expr(lexer,tc,eb);
		
		Expr_Index expr_i = expr_buffer_alloc(eb);
		
		Expr *expr = expr_buffer_at(eb,expr_i);
		// now we know it is not just a cell or a number but rather an expression
		
		memset(expr,0,sizeof(Expr));
		
		expr->kind = EXPR_KIND_BOP;
		expr->as.bop.lhs = lhs_i;
		expr->as.bop.rhs = rhs_i;
		
		
		if(sv_eq(token,SV("+"))){
			printf("sadsadasds\n");
			expr->as.bop.kind = BOP_ADD;
		}
		else if(sv_eq(token,SV("*"))){
			expr->as.bop.kind = BOP_MUL;
		}
		else if(sv_eq(token,SV("-"))){
			expr->as.bop.kind = BOP_SUB;
		}
		else if(sv_eq(token,SV("/"))){
			expr->as.bop.kind = BOP_DIV;
		}
		else{
			printf("asdokpsajdposajdoasd\n\n");
		}
		return expr_i;

	}

	return lhs_i; // base case the current rhs in case of binary expr is the lhs of the next sub_binary expr;
	
}
// what are we doing well we are trying to parse the binary operation of expression 
// we only take care of addition 
// if lhs only then return it as a `name`
// else (there is a `+` )
// so we evaluate the rhs as another `+` expr recusively
// at the end when the count is 0 (we are consuming the src in the parsing phase)
// we just return the expr of lhs and rhs
// in case only lhs exists we return it 



void dump_expr(FILE *stream,Expr_Buffer *eb,Expr_Index expr_i,int level){ // we are basically dealing with a tree

	fprintf(stream,"%*s",level*2,"");
	Expr *expr = expr_buffer_at(eb,expr_i);
	switch (expr->kind){
		case EXPR_KIND_NUMBER:
			fprintf(stream,"NUMBER: %lf\n",expr->as.number);
			break;
		case EXPR_KIND_CELL:
			fprintf(stream,"CELL (%zu , %zu)\n",expr->as.cell.col,expr->as.cell.row);
			break;
		case EXPR_KIND_BOP:{
			fprintf(stream,"ADD: \n");
			dump_expr(stream,eb,expr->as.bop.lhs,level+1);
			dump_expr(stream,eb,expr->as.bop.rhs,level+1);
		
		}break;
		default:
			fprintf(stream,"ERROR: unrecognized Expr Kind\n");
			exit(1);


	}
	

}


Expr_Index parse_expr(Lexer_Input *lexer,Tmp_Cstr *tc,Expr_Buffer *eb){


// 	while(src.count>0){
//	String_View token = next_token(&src);
//	printf(SV_Fmt"\n",SV_Arg(token));	
//	}
//	assert(0 && "NOT IMPLEMENTED YET");



	return parse_bop_expr(lexer,tc,eb);
	
}


Table table_alloc(const char* file_path,size_t rows,size_t cols){
	Table table = {
		.file_path = file_path,
	};
	table.rows = rows;
	table.cols = cols;

	table.cells = malloc(sizeof(Cell)*rows*cols);
	if(table.cells==NULL){
		fprintf(stderr,"ERROR: Insufficient memory\n");
		exit(1);
	}
	// Why are we initing it with zero u may ask
	// well if it was a global or a static variable we wouldnt need to
	// however it is a local variable allocated in the heap therefor it won't be initialised to zero by default
	// and this is for sure run at runtime without betting on what the compiler will do
	memset(table.cells,0,sizeof(Cell)*rows*cols);
	// now we are init the Cells with 0s (again it is still a valid struct)

	return table;
}


// now we have boundary checking
Cell *table_cell_at(Table *table,Cell_Index index){
	assert(index.row < table->rows);
	assert(index.col < table->cols);
	return &table->cells[index.row * table->cols + index.col];
}
// our table is stored like this : [A0,B0,C0,A1,B1,C1,A2,B2,C2];
char *cell_kind_as_cstr(Cell_Kind kind){
	switch(kind){
		case CELL_KIND_TEXT:
			return "TEXT";
		case CELL_KIND_NUMBER:
			return "NUMBER";
		case CELL_KIND_EXPR:
			return "EXPR";
		case CELL_KIND_CLONE:
			return "CLONE";
		default:
			assert(0 && "UNREACHABLE"); // this should never exec 
			exit(1);
	}
}

void dump_table(FILE* stream,Table* table){
	for(size_t row = 0;row<table->rows;++row){
		for(size_t col=0;col<table->cols;++col){
			printf("%zu,%zu\t;",row,col);
			Cell_Index cell_index = {
				.row = row,
				.col = col,
			};
			Cell *cell = table_cell_at(table,cell_index);
			fprintf(stream,"%s : %zu :%zu : %s\n",table->file_path,cell->file_row,cell->file_col,cell_kind_as_cstr(cell->kind));
		}
	}
}

void parse_table(Table *table,Expr_Buffer *eb,Tmp_Cstr *tc,String_View content){
// lets do this
//	size_t row = 0;
//	size_t col = 0;
	Lexer_Input *lexer = malloc(sizeof(Lexer_Input));
	memset(lexer,0,sizeof(Lexer_Input));
	lexer->file_path = table->file_path;


	for( size_t row  = 0; content.count > 0;++row){
		String_View line = sv_chop_by_delim(&content,'\n');
		const char *line_start = line.data;
		lexer->line_start = line_start;
		lexer->file_row = row + 1;
		for(size_t col=0;line.count > 0;++col){
			
		// Remember you made the table_cell_at Func to access the table with boundary checking
		//	table->cells[row*table->cols+col] = cell;
		//	col ++;
			String_View cell_data = sv_trim(sv_chop_by_delim(&line,'|'));
			Cell_Index cell_index = {
				.row = row,
				.col = col
				};

			
			Cell *cell = table_cell_at(table,cell_index);
			cell->file_row = row;//file line is the same as the row
			cell->file_col = cell_data.data - line_start;// the char in the line needs to be calculated through ptrs

			if(sv_starts_with(cell_data,SV("="))){
				sv_chop_left(&cell_data,1);
				cell->kind = CELL_KIND_EXPR;
				// and here we would use malloc : 
				// cell->as.expr = malloc(sizeof(Cell_Expr));
				// and then we have access to the expr Struct ptr
				lexer->src = cell_data;

				cell->as.expr.index = parse_expr(lexer,tc,eb); // if the Cell_Expr was a pointer then we would have to use malloc otherwise we would get a SEGMENTATION FAULT (inited with NULL)
			}
			else if(sv_starts_with(cell_data,SV(":"))){
				sv_chop_left(&cell_data,1);
				lexer->src = cell_data;

				cell->kind  = CELL_KIND_CLONE;
				if(sv_eq(cell_data,SV("<"))){
					cell->as.clone = DIR_LEFT;
				}
				else if(sv_eq(cell_data,SV(">"))){
					cell->as.clone = DIR_RIGHT;
				}
				else if(sv_eq(cell_data,SV("^"))){
					cell->as.clone = DIR_UP;
				}
				else if(sv_eq(cell_data,SV("v"))){
					cell->as.clone = DIR_DOWN;
				}else{
					fprintf(stderr,"%s : %zu : %zu : ERROR:"SV_Fmt" is not a correct dir",table->file_path,cell->file_row,cell->file_col,SV_Arg(cell_data));
					exit(1);
				}
			}
			else{

				// Since we only need this buffer in here and we want it to be allocated in the data section we 
				// add static so it wont liberate and we still use it
			//	static char tmp_buffer[1024 * 4 ];
			//
			//	assert(cell_data.count < sizeof(tmp_buffer));
			//	snprintf(tmp_buffer,
			//	sizeof(tmp_buffer),
			//	SV_Fmt,
			//	SV_Arg(cell_data)
			//	);	

				// now we try to convert the string to double 
				// we use strtod(char* start_ptr,char* end_ptr)
				// if end_ptr == start_ptr OR *end_ptr != '\0' Then the conversion was unsuccessfull
				// if end_ptr != start_ptr OR *end_ptr == '\0' Then the conversion was SUCCESSFULL
				
			//	char *end_ptr ;
			//	cell->as.number = strtod(tmp_buffer,&end_ptr);

				if(sv_strtod(cell_data,tc,&cell->as.number)){
					cell->kind = CELL_KIND_NUMBER;
				}else{
					cell->kind = CELL_KIND_TEXT;
					cell->as.text = cell_data;
				}

				 
			}
			//cell->as.text = cell_data;
		
		}
	//	row ++;
	}	
}



void print_table(Table table,Expr_Buffer *eb){
	for(size_t row=0;row < table.rows ;++row){
		for(size_t col=0;col < table.cols;++col){
			// printf("CELL : (%zu , %zu) : ",row,col);
			Cell_Index cell_index = {
				.row = row,
				.col = col
			};
			Cell *cell = table_cell_at(&table,cell_index);
			switch(cell->kind){
				case CELL_KIND_TEXT:
					printf(SV_Fmt,SV_Arg(cell->as.text));
					break;
				case CELL_KIND_NUMBER:
					printf("%lf",cell->as.number);
					break;
				case CELL_KIND_EXPR:
					{
						// printf("EXPR :");
						if(cell->status == EVALUATED){
							printf("%lf",cell->as.expr.value);
						}
						else{
						printf("\n");
						dump_expr(stdout,eb,cell->as.expr.index,1);
						}
						
					}
					break;	
				case CELL_KIND_CLONE:
				{
					fprintf(stdout,"%d",cell->as.clone+10);
				}				
			}
			if(col < table.cols-1)	printf("|");
		//	printf("%s|",cell_kind_as_cstr(cell->kind));
		}
		printf("\n");
	}
}


void usage(FILE *stream)
{
	// i like this way of handling user miss-use
	fprintf(stream,"Usage: ./main <input.csv>\n");
}



void expr_buffer_dump(FILE *stream,const Expr_Buffer *eb,Expr_Index *root){
	fwrite(root,sizeof(root),1,stream);
	fwrite(&eb->count,sizeof(eb->count),1,stream);
	// what element , size of element , how many elements,where
	fwrite(eb->items,sizeof(Expr),eb->count,stream);
}


char *move_into_memory(const char *file_path, size_t *size) // obviously we dont want to change the file_path
// so we use const char for it and not the same feeling for size
{
	// Quick summary : 
	// we get the stream of a file
	// we determine the size of a file
	// we allocate corresponding size in the buffer
	// we move it in the buffer
	// we return the buffer

	char *buffer = NULL; // the buffer we are going to return and we will deallocate it if we encounter any error
	(void) file_path; // this is used to suppress compiler warnings about unused Vars 
	(void) size;

	FILE *f = fopen(file_path,"rb");
	if(f==NULL){
		goto error;
	}
	// now we have a stream to the file we can use it to move the content to memory
	
	// TO DETERMINE THE SIZE OF A STREAM : ftell fseek (cursor end => position == number of bytes)
	if(fseek(f,0,SEEK_END) < 0){
		goto error;
	}

	long m = ftell(f); // we take the value of the cursor of the file f

	if (m<0){
		goto error;
	}
	// so now we also ehave the file's Size so we can alocate the corresponding buffer in memory
	buffer = malloc(sizeof(char)*m); 
	if(!buffer){
		goto error;
	}
	if(fseek(f,0,SEEK_SET)<0){
		goto error;
	}
	size_t n = fread(buffer,
					1,// tis is the size of the element i am reading (i have to provide size of my struct in case
					// i was using one )
					m, // the size of the file
					f // the stream
			); // if an error happens it does not return it through output but rather setting other flags

	assert(n== (size_t)m); // cast m to size_t and assert we read all




	if (ferror(f)){ // if a flag of error has been set we go to error
		goto error;
	}

	if (size){ // if user requested the size (kinda of optional param if we pass NULL)
		*size = n; 
	}
	fclose(f);
	
//	assert(0 && "Not Implementd"); // a nice way to abstract modules implementations but assert that 
					// I come back ; 
// Now after i implemented the function i comment it
	return buffer;
	

error:
	if(f){
		fclose(f);
	}
	if(buffer){
		free(buffer);
	}
	return NULL;
	// apparently this is simulating something called as defer() which is a routine that pushes a function into 
	// the callable fcts list when the caller function returns (basically a primitive garbage collector and error handler)
}

void estimate_table_size(String_View content, size_t *out_rows,size_t *out_cols){

	size_t rows =0;
	size_t cols = 0;
	// we need the scope of row and col variable to be visible in the whole function
	for(; content.count>0 ; ++rows){
		String_View line = sv_chop_by_delim(&content,'\n');
		//const char *start = content.data;
		size_t col = 0;
		for(; line.count>0 ; ++col){
			sv_chop_by_delim(&line,'|');
		}
		if(col > cols) cols = col;
	}

	if(out_rows){ // if he asks for it by giving an actual pointer
		*out_rows = rows;
	}
	if(out_cols){
		*out_cols = cols;
	}
	
}
void evaluate_table_cell(Table *table,Expr_Buffer *eb,Cell_Index cell_index);

double evaluate_table_expr(Table *table,Expr_Buffer *eb,Expr_Index expr_i,Cell_Index cell_index){
	Expr *expr = expr_buffer_at(eb,expr_i);
	switch(expr->kind){// So now that we introduced the `CLONE` cmd we need to handle the conversion from a meta cmd to an expression or a what ever the pointer is pointing at
	// so we try to evaluate the cell before evaluating the expression
	// why ?! u might ask 
	// well simply `we want to determine what to evaluate b4 evaluate it`
	// in other words if the cell is pointing directly or undirectly to an expression we want to copy the expression's significance to the current cell and then we evaluate the xpr
	// when we try to eval the expr we have no idea that it is a clone or not ; such thing does not exist in its dictionnary 
		case EXPR_KIND_NUMBER: return expr->as.number;
		case EXPR_KIND_CELL:{
			evaluate_table_cell(table,eb,expr->as.cell);
			Cell *cell = table_cell_at(table,expr->as.cell);

			
			switch(cell->kind){
				case CELL_KIND_NUMBER:
					return cell->as.number;
				case CELL_KIND_TEXT:{
					fprintf(stderr,"ERROR:%s : %zu : %zu : text cells ( %zu , %zu )cant be evaluated in the expression\n",table->file_path,cell_index.row,cell_index.col,cell->file_row,cell->file_col);
				}break;
				case CELL_KIND_EXPR :
					//evaluate_table_cell(table,eb,expr->as.cell);
					return cell->as.expr.value;
				
				case CELL_KIND_CLONE:
					assert(0 && "UNREACHABLE");
					break;
			}

		}break;
		
		
		case EXPR_KIND_BOP:{
			// we evaluate lhs and rhs and add them
			
			double lhs = evaluate_table_expr(table,eb,expr->as.bop.lhs,cell_index);
			double rhs = evaluate_table_expr(table,eb,expr->as.bop.rhs,cell_index);
			// here when we want to support multiple operations we just do a switch and return the corresp.result
			switch (expr->as.bop.kind){
			case BOP_ADD:
				return lhs + rhs;
			case BOP_DIV:
				return lhs / rhs;
			case BOP_MUL:
				return lhs * rhs;
			case BOP_SUB:
				return lhs - rhs;


			}
		} break;

		
	}
	// default:
		//assert(0&& "UNREACHABLE");
		return 0.0;
}

Cell_Index adjacent_cell(Cell_Index cell_index,Dir dir){
	switch (dir){
		case DIR_LEFT:
		
			cell_index.col -= 1;
		
		break;
		case DIR_DOWN:
			cell_index.row +=1;
			break;
		case DIR_RIGHT:
			cell_index.col +=1;
			break;
		case DIR_UP:
			cell_index.row -=1;
			//fprintf(stdout,"RETURNED UP CELL INDEX\n");
			break;
		default:{
		assert(0 && "UNREACHABLE IN THE ADJACENT CELL FUNC");
		exit(1);}
	}

	return cell_index;
}

Dir opposite_dir(Dir dir){
	switch (dir){
		case DIR_DOWN:
			return DIR_UP;
		case DIR_UP:
			return DIR_DOWN;
		case DIR_LEFT:
			return DIR_RIGHT;
		case DIR_RIGHT:
			return DIR_LEFT;
		default:{
		assert(0 && "UNREACHABLE");
		exit(1);}
	}
}

Expr_Index mv_expr(Expr_Buffer *eb,Expr_Index expr_index,Dir dir){
	Expr *expr = expr_buffer_at(eb,expr_index);
	switch(expr->kind){
		case EXPR_KIND_NUMBER:
		//printf("EXPR_INDEX: %zu\n",expr_index);
		return expr_index;
		case EXPR_KIND_CELL:
		{
			Expr_Index new_expr_index = expr_buffer_alloc(eb);
			Expr *new_expr = expr_buffer_at(eb,new_expr_index);

			new_expr->kind = EXPR_KIND_CELL;
			
			new_expr->as.cell = adjacent_cell(expr->as.cell,dir);
			//printf("EXPR_INDEX: %zu\n",new_expr_index);
			return new_expr_index;
		}break;
		case EXPR_KIND_BOP:
		{
			Expr_Index new_expr_index = expr_buffer_alloc(eb);
			Expr *new_expr = expr_buffer_at(eb,new_expr_index);

			new_expr->kind = EXPR_KIND_BOP;
			new_expr->as.bop.lhs = mv_expr(eb,expr->as.bop.lhs,dir);
			new_expr->as.bop.rhs = mv_expr(eb,expr->as.bop.rhs,dir);
			//printf("EXPR_INDEX: %zu\n",new_expr_index);
			return new_expr_index;
		}
		break;
		default:{
		assert(0 && "UNREACHABLE");
		exit(1);}
	}
}




void evaluate_table_cell(Table *table,Expr_Buffer *eb,Cell_Index cell_index){
	if(!(table )){
		fprintf(stderr,"ERROR: NULL pointer was passed at evaluate table cell");
		exit(1);
	}
	Cell *cell = table_cell_at(table,cell_index);

	switch (cell->kind){
	// printf("AN ITERATION IN CELL EVAL FOO");
	case CELL_KIND_TEXT:
	case CELL_KIND_NUMBER:
		cell->status = EVALUATED;
		break; // cool trick if two cases have same execution lines just use a single break;
	case CELL_KIND_EXPR :{
			switch(cell->status ){
				
				case INPROGRESS:{
				fprintf(stderr,"ERROR: %s : %zu : %zu : Circulair dependency (DEADLOCK)\n ",table->file_path,cell->file_row,cell->file_col);
				exit(1);
				}break;
				
				case UNEVALUATED:{
						cell->status = INPROGRESS;
						cell->as.expr.value = evaluate_table_expr(table,eb,cell->as.expr.index,cell_index);
						cell->status = EVALUATED;
				}break;
				case EVALUATED:break;
			}
			break;
			// cell->as.expr.status = true;
			// cell->as.expr.value = evaluate_table_expr(table,cell->as.expr.ptr);
		}break;
	case CELL_KIND_CLONE : {
			if (cell->status == INPROGRESS){
				fprintf(stderr,"ERROR: %s : %zu : %zu : Deadlock detected \n",table->file_path,cell->file_row,cell->file_col);
				exit(1);
			}
			if (cell->status == UNEVALUATED){
				cell->status = INPROGRESS;
				Dir dir = cell->as.clone;
				Cell_Index adj_index = adjacent_cell(cell_index,dir);
				evaluate_table_cell(table,eb,adj_index);
				Cell *adj_cell = table_cell_at(table,adj_index);

				cell->kind = adj_cell->kind;
				cell->as = adj_cell->as;
				
				if(cell->kind == CELL_KIND_EXPR){
					cell->as.expr.index = mv_expr(eb,cell->as.expr.index,opposite_dir(dir));
					//cell->status = UNEVALUATED;
					cell ->as.expr.value = evaluate_table_expr(table,eb,cell->as.expr.index,cell_index);
				}
				cell->status = EVALUATED;
//				fprintf(stdout,"This is the data ?: %lf\n",cell->as.expr.value);
				// the problem was a missing `break;`
				//printf("I AM UNEVALUATED CELL!\n");
				//return;
			}
			else{
				assert(0 && "UNREACHABLE EVALUATED CLONE");
				exit(1);
			}
			


//			assert(0 && "UNIMPLEMENTED");


	/*		Cell_index adjacent_index = adjacent_cell	(cell_index,cell->as.clone);
			evaluate_table_cell(table,eb,adjacent_index);
			Cell *adjacent_cell = table_cell_at(table,adjacent_index);
	*/
		}
	break;
	default:{
		assert(0 && "UNREACHABLE");
		exit(1);
	}
	}
}

void evaluate_table(Table *table,Expr_Buffer *eb){
	for(size_t row = 0;row<table->rows;++row){
		for(size_t col = 0;col<table->cols;++col){
			Cell_Index cell_index = {
				.row = row,
				.col = col
			};
			//Cell *cell =table_cell_at(table,cell_index);
			evaluate_table_cell(table,eb,cell_index);
		}
	}
}

int main(int argc,char **argv)
{
	if(argc<2){ // user (I) didnt provide input file
	usage(stderr);
	fprintf(stderr,"ERROR: Required arguments were not provided ! PLEASE COOPERATE!!!");
	exit(1);
	}

	const char *file_path = argv[1];  // it is supposed to be the first argument to be passed
	size_t content_size = 0; // always remember your semi-colon
	char *content = move_into_memory(file_path,&content_size); // passing the pointer has a name i studied 
								// as a first year CS student but i forgor ; just so we have write access to it

	if(content == NULL) {
	fprintf(stderr,"ERROR: could not read file %s : %s\n",
		file_path,
		strerror(errno));
	exit(1);
	}
	// right now im using someone else library for parsing but i might create mine in the near future


	String_View input = {
		.count = content_size,
		.data = content
	};

	//for (size_t row=0 ; input.count>0 ; ++row){ // we get line by line and split it 

	//	String_View line = sv_chop_by_delim(&input,'\n') ;// we get the line
	//	const char *start = line.data; // the pointer where the line starts
		
		//for (size_t col=0 ; line.count>0 ; ++col){
			// Now we get each cell in that row and we print it : filename:row,chart_start_pos,coords,data

			//String_View cell  = sv_trim(sv_chop_by_delim(&line,'|')); // the line will be modified in this library
			// thats why i need to save it in then	 ```start``` 
			// cell.data the pointer where the next col in the row starts
			
//			printf("%s:%zu:%zu: (%zu , %zu) "SV_Fmt"\n",  // %zu for size_t
//				file_path,
//				row,
//				cell.data-start,
//				row,
//				col,
//				SV_Arg(line)); 

	//	size_t rows,cols;
	/*	estimate_table_size


		}
	}
	*/
	Expr_Buffer eb = {0};
	size_t rows, cols;
	estimate_table_size(input,&rows,&cols);
	printf("Size of Our Table : %zux%zu\n",rows,cols);
//	fwrite(content,1,content_size,stdout); // basically printing the file in the console
//	Cell *table = malloc(sizeof(Cell) * rows * cols);
//	if (table==NULL){
//		fprintf(stderr,"ERROR: Insufficient memory");
//		exit(1);
//	}
//	memset(table,0,sizeof(Cell)*rows*cols);
	// Putting everythin inside corresponding function is good but i don't see it as helpful when first trying to 
	// implement the idea in other words taking that extra step to generalize or parametarize a routine might 
	// just halt you for no good reason
	Table table = table_alloc(file_path,rows,cols);
	Tmp_Cstr tc = {0};
	parse_table(&table,&eb,&tc,input);
	//print_table(table,&eb);

	evaluate_table(&table,&eb);
	print_table(table,&eb);
//	dump_table(stdout,&table);
	// String_View src = SV_STATIC("A1 +B2+69+C1+D3");
	// printf("I was reached\n");
	// Expr *expr = parse_expr(&src);
	// dump_expr(stdout,expr,0);

// THIS IS USED FOR SERIALIZATION
//	FILE *f = fopen("Expr.bin")
// expr_buffer_dump(f,&eb,&root);

// THIS IS USED FOR DE-SERIALIZATION
// FILE *file = fopen("Expr.bin","rb");
// Expr_Index root = 0;
// fread(&root,sizeof(root),1,file);
// size_t count = 0;
// fread(&count,sizeof(count),1,file);
// Expr_Buffer eb = {0};
// eb.count = count;
// eb.capacity = count; setting the cap to count as well
// eb.items = malloc(sizeof(Expr)*eb.count);
// fread(eb.items,sizeof(Expr),eb.count,file);


// fclose(file);


	free(content); // `remove` file contnent from the RAM
	free(table.cells);// free our table of cells
	free(eb.items); // Free the expressions array and we traverse NOTHING thanks to `slab allocation`
	free(tc.cstr); // free the buffer used for conversions in parsing in [sv_strotod,sv_strtol];

	return 0;

	// refactoring is SH!T ;D

}
