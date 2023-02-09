
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

typedef enum{
	EXPR_KIND_NUMBER = 0,
	EXPR_KIND_CELL,
	EXPR_KIND_ADD,
}Expr_Kind;


typedef struct{
	Expr *lhs;
	Expr *rhs;
}Expr_Add;


typedef struct{

	size_t row;

	size_t col;
	
}Expr_Cell; // if the contetnt of expr is ex C12 we split it into (C,12) col and row

typedef union{

	double number;
	Expr_Cell cell;

	Expr_Add add;
	
}Expr_As;


typedef struct Expr{
	Expr_Kind kind;
	Expr_As as;
};

typedef enum{
	CELL_KIND_TEXT = 0,
	CELL_KIND_NUMBER,
	CELL_KIND_EXPR,
}Cell_Kind;

// well recall a union is basically a vraiable can host many types ( we allocate the biggest size of the types
// we might use and treat it as any other pointer in the world ) ;D  

typedef union{
	String_View text;
	double number;
	Expr *expr;
} Cell_As;


// i think it is good practice to call unions NAME_As for domain specific language


// 0 initialising a struct is still a valid struct
typedef struct{
	Cell_Kind kind;
	Cell_As as;
	
}Cell;

typedef struct {
	Cell *cells;
	size_t rows;
	size_t cols;
}Table;


bool is_name(char c){

	return isalnum(c)|| c=='_';
	
}

// int isdigit(int) different signature so we need to implement a worarround but is duper simple
bool is_digit(char c){
	return isdigit(c);
}

String_View next_token(String_View *src){
	*src = sv_trim(*src);

	if(src->count == 0){
		return SV_NULL;
	}
	if(*src->data=='='){
		return sv_chop_left(src,1);// we take 1 char
	}
	if (*src->data=='+'){
		return sv_chop_left(src,1); // take one character out from the left and return it (the chopped char)
	}
	if(is_digit(*src->data)){
		return sv_chop_left_while(src,is_digit); // takes a predicate ( func pointer )
	}
	if (is_name(*src->data)){
		return sv_chop_left_while(src,is_name);
	}
	fprintf(stderr,"ERROR: unknown token starts with %c",*src->data);
	exit(1);
}


Expr *parse_expr(String_View src){


 	while(src.count>0){
	String_View token = next_token(&src);
	printf(SV_Fmt"\n",SV_Arg(token));
 		
 	}
//	assert(0 && "NOT IMPLEMENTED YET");
	return NULL;
	
}
Table table_alloc(size_t rows,size_t cols){
	Table table = {0};
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
Cell *table_cell_at(Table *table,size_t row,size_t col){
	assert(row < table->rows);
	assert(col < table->cols);
	return &table->cells[row * table->cols + col];
}




void parse_table(Table *table,String_View content){
// lets do this
//	size_t row = 0;
//	size_t col = 0;

	for( size_t row  = 0; content.count > 0;++row){
		String_View line = sv_chop_by_delim(&content,'\n');
		
		for(size_t col=0;line.count > 0;++col){
			
		// Remember you made the table_cell_at Func to access the table with boundary checking
		//	table->cells[row*table->cols+col] = cell;
		//	col ++;
			String_View cell_data = sv_trim(sv_chop_by_delim(&line,'|'));
			
			Cell *cell = table_cell_at(table,row,col);
			if(sv_starts_with(cell_data,SV("="))){
				cell->kind = CELL_KIND_EXPR;
				cell->as.expr = parse_expr(cell_data);
			}
			else{

				// Since we only need this buffer in here and we want it to be allocated in the data section we 
				// add static so it wont liberate and we still use it
				static char tmp_buffer[1024 * 4 ];
			
				assert(cell_data.count < sizeof(tmp_buffer));
				snprintf(tmp_buffer,
				sizeof(tmp_buffer),
				SV_Fmt,
				SV_Arg(cell_data)
				);	

				// now we try to convert the string to double 
				// we use strtod(char* start_ptr,char* end_ptr)
				// if end_ptr == start_ptr OR *end_ptr != '\0' Then the conversion was unsuccessfull
				// if end_ptr != start_ptr OR *end_ptr == '\0' Then the conversion was SUCCESSFULL
				
				char *end_ptr ;
				cell->as.number = strtod(tmp_buffer,&end_ptr);

				if(end_ptr != tmp_buffer && *end_ptr == '\0'){
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
char *cell_kind_as_cstr(Cell_Kind kind){
	switch(kind){
		case CELL_KIND_TEXT:
			return "TEXT";
		case CELL_KIND_NUMBER:
			return "NUMBER";
		case CELL_KIND_EXPR:
			return "EXPR";
		default:
			assert(0 && "UNREACHABLE"); // this should never exec 
			exit(1);
	}
}


void print_table(Table table){
	for(size_t row=0;row < table.rows ;++row){
		for(size_t col=0;col < table.cols;++col){
			Cell *cell = table_cell_at(&table,row,col);
			switch(cell->kind){
				case CELL_KIND_TEXT:
					printf("TEXT("SV_Fmt")",SV_Arg(cell->as.text));
					break;
				case CELL_KIND_NUMBER:
					printf("NUMBER(%lf)",cell->as.number);
					break;
				case CELL_KIND_EXPR:
					printf("EXPR(***)");
					break;					
			}
			printf("|");
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
	Table table = table_alloc(rows,cols);
	parse_table(&table,input);
	print_table(table);

	


	return 0;

}
