
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>



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

	fwrite(content,1,content_size,stdout); // basically printing the file in the console
	return 0;

}
