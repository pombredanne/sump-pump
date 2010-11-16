/*
 *    Copyright 1996-2007  Ordinal Technology Corp.  All Rights Reserved.
 *
 *	Nsort.h	- API for the nsort subroutine library
 *
 *
 *  The typical sequence of Nsort API calls is as follows:
 *	nsort_define(nsort_sort_def, 0, NULL, &context);
 *	for each record or block of records to be sorted
 *		nsort_release_recs(buf, size, &context);
 *	nsort_release_end(&context);
 *	for each record or block of records returned from sort
 *		nsort_return_recs(buf, &size, &context);
 *	nsort_print_stats(&context, fp);	// optional
 *	nsort_end(&context);
 *
 *  See per-function documentation below for more details.
 */
#ifndef _NSORT_H_
#define _NSORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nsorterrno.h"

typedef unsigned nsort_t;	/* nsort context identifier */
typedef int nsort_msg_t;	/* return status & error message numbers */

/*
 * Error handling
 *	When nsort detects a fatal error or a non-fatal warning condition 
 *	it invokes a message handling function.
 *	A sort may assign its own message handler in nsort_define()
 *	This handler must return (rather than, e.g. executing a longjmp()) so
 *	that the sort's resources will be correctly freed.
 *	The error may occur during parsing, in which case the definition string
 *	will be non-null; offset indicates where it detected the error
 *	in the definition string
 *	The error may have several mesage-specific arguments, passed as char *.
 *	ndetails may be zero.
 */
typedef void	(*nsort_error_t)(void *arg, nsort_msg_t msg, nsort_t context,
					    char *definition, int offset,
					    int ndetails, ...);
typedef int	(*nsort_compare_t)(void *p1, void *p2, int len1, int len2,
				   void *compare_arg);
typedef int	(*nsort_merge_input_t)(int merge_index, char *buf, int size,
				       void *input_arg);

typedef struct
{
	nsort_error_t	error;
	void		*arg;	/* pointer-sized value to pass to error func */
} nsort_error_callback_t;
#define nsort_callback_t nsort_error_callback_t /* support original type name*/

typedef struct
{
	nsort_merge_input_t	input;
	void		        *arg;	/* pointer-sized value to pass to
					   merge input func */
} nsort_merge_callback_t;


/* nsort_define - Allocate and initiailize a sort based on the
 *		  nsort definition statements, options, and file names
 *		  given in the sortdef string.
 * Arguments:
 *	def -		Pointer to string containing the sort definition
 *			in the Nsort command language.
 *	options -	Unsigned int containing possible bit options.  
 *			Possible options are as follows:
 *		NSORT_NO_DEFAULTS 
 *			If specified, Nsort will not read the nsort.params
 *			file to get any system-wide configuration defaults.
 *	error_callbacks - Pointer to nsort_error_callback_t structure
 *			containing error callback routine and user-specified
 *			argrument, or a NULL pointer (no error callback).
 *	ctxp -		Pointer to sort context id (unsigned int).  The
 *			current implementation always overwrites this
 *			id with a new sort context id.
 *
 * Returns:
 *	NSORT_SUCCESS   Operation completed.
 *
 */
extern nsort_msg_t nsort_define(const char *def,
				unsigned options,
				nsort_error_callback_t *callbacks,
				nsort_t *ctxp);

#define NSORT_NO_DEFAULTS	0x0001	/* do not read nsort.params */


/* nsort_merge_define - Allocate and initiailize a merge based on the
 *		        nsort definition statements, and options
 *		        given in the sortdef string.
 * Arguments:
 *	def -		Pointer to string containing the merge definition
 *			in the Nsort command language.
 *	options -	Unsigned int containing possible bit options.  
 *			Possible options are as follows:
 *		NSORT_NO_DEFAULTS 
 *			If specified, Nsort will not read the nsort.params
 *			file to get any system-wide configuration defaults.
 *	error_callbacks - Pointer to nsort_error_callback_t structure
 *			containing error callback routine and a user-specified
 *			argrument, or a NULL pointer (no error callback).
 *	merge_width -	Total number of merge input files and callbacks. > 0
 *	merge_input -	Pointer to nsort_merge_callback_t structure and a
 *			user-specified argument.
 *	ctxp -		Pointer to sort context id (unsigned int).  The
 *			current implementation always overwrites this
 *			id with a new sort context id.
 *
 * Returns:
 *	NSORT_SUCCESS   Operation completed.
 *
 */
extern nsort_msg_t nsort_merge_define(const char *def,
				      unsigned options,
				      nsort_error_callback_t *callbacks,
				      int merge_width,
				      nsort_merge_callback_t *merge_input,
				      nsort_t *ctxp);


/* nsort_release_rec{ord,s} - Release a record or records to nsort for
 *			      sorting.  Nsort_release_recs() can be
 *			      used to release multiple records at a
 *			      time.  Nsort_release_record() is 
 *			      currently the same as nsort_release_recs(),
 *			      but in the future will take a maximum of
 *			      1 record. 
 * Arguments:
 *	buf -		Pointer to record(s) being released to nsort.
 *	size -		Size of record(s).
 *	ctxp -		Pointer to sort context id.
 *
 * Returns:
 *	NSORT_SUCCESS   Operation completed.
 *	NSORT_INVALID_PHASE
 *			Nsort_release_end() has already been called.
 *      NSORT_INVALID_CONTEXT
 *			The context id is invalid.
 */
#define nsort_release_record(a,b,c)  nsort_release_recs((a),(b),(c))
extern nsort_msg_t nsort_release_recs(void *buf, 
				      size_t size, 
				      nsort_t *ctxp);


/* nsort_release_end - indicate to nsort that there are no further
 *			records to be released.  An nsort_release_end()
 *			call is required between the last
 *			nsort_release_rec{s,ord}() call and the first
 *			nsort_return_rec{s,ord}() call.
 * Arguments:
 *	ctxp -		pointer to a context id.
 *
 * Returns:
 *	NSORT_SUCCESS   Operation completed.
 *      NSORT_INVALID_CONTEXT
 *			The context id is invalid.
 */
extern nsort_msg_t nsort_release_end(nsort_t *ctxp);


/* nsort_return_rec{ord,s} - Request records to be returned from nsort
 *			     in sorted order.  Nsort_return_recs() can be
 *			     used to return multiple records at a time.
 *			     Nsort_return_record() is currently the same
 *			     as nsort_return_recs(), but in the future
 *			     will return a maximum of 1 record.
 * Arguments:
 *	buf -		Pointer to buffer where returned record(s) 
 *			should be placed.
 *	size -		Pointer to an integer containing the size of 
 *			the buffer.  This integer will be modified
 *			by the function to contain the actual size
 *			of the record(s) returned in the buffer.
 *	ctxp -		Pointer to sort context id.
 *
 * Returns:
 *	NSORT_SUCCESS   One record (or more for nsort_return_recs())
 *			has been returned in the buffer.  The size
 *			of the record(s) is indicated in the size
 *			argument.
 *	NSORT_END_OF_OUTPUT  
 *			The end of sort output has been reached.
 *			No records were returned by this call.
 *	NSORT_INVALID_PHASE
 *			nsort_release_end() has not yet been called.
 *      NSORT_INVALID_CONTEXT
 *			The context id is invalid.
 */
#define nsort_return_record(a,b,c)  nsort_return_recs((a),(b),(c))
extern nsort_msg_t nsort_return_recs  (void *buf,
				       size_t *size, 
				       nsort_t *ctxp);


/* nsort_print_stats - Print the nsort statistics output for a completed 
 *			sort to the given file pointer.
 *			This function may be called only after a call to 
 *			nsort_return_recs() that returns NSORT_END_OF_OUTPUT,
 *			and before a call to nsort_end().
 * Arguments:
 *	ctxp -		Pointer to a context id.
 *
 * Returns:
 *	NSORT_SUCCESS   Operation completed.
 *	NSORT_INVALID_PHASE
 *			Nsort has not yet returned the final record.
 */
extern nsort_msg_t nsort_print_stats(nsort_t *ctxp, FILE *fp);

/* nsort_get_stats -	String version of nsort_print_stats.
 *			This function may be called only after a call to 
 *			nsort_return_recs() that returns NSORT_END_OF_OUTPUT,
 *			and before a call to nsort_end().
 * Arguments:
 *	ctxp -		Pointer to a context id.
 *
 * Returns:
 *	NULL		Nsort has not yet returned the final record, or other
 *			error.
 *	<string>	A multi-line ascii string. Nsort will free the string
 *			when the application calls nsort_end();
 */
const char *nsort_get_stats(nsort_t *ctxp);

/* nsort_message - return pointer to the error message string for the
 *		   most recent error issued by the given Nsort API 
 *		   context id.
 * Arguments:
 *	ctxp -		Pointer to a context id.
 *
 * Returns:
 *	Pointer to error message string. free() should not be called on
 *	the return value, as the storage may not have been allocated
 *	by malloc().
 */
extern char *nsort_message(nsort_t *ctxp);


/* nsort_end - Cancel and terminate a sort.  This call cancels any sort 
 *		in progress, frees any resources allocated for the 
 *		given context memory, and deallocates the context.
 * Arguments:
 *	ctxp -		Pointer to a context id.  The id is cleared
 *			by the function before it returns.
 *
 * Returns:
 *	NSORT_SUCCESS   Operation completed.
 *      NSORT_INVALID_CONTEXT
 *			The context id is invalid.
 */
extern nsort_msg_t nsort_end(nsort_t *ctxp);

/* nsort_declare_function -	give a user-specific comparison function a
 *			 	name to use in a field or key definition.
 *
 *	Nsort keeps a global table of functions which may be used in
 *	user-defined key comparisons. Once declared, the given name is used
 *	in a field or key definition with the "compare=<name>" qualifier.
 *
 *	When invoked by nsort, the comparison function will be called with
 *	five arguments:
 *		void *col1
 *		void *col2
 *		int len1
 *		int len2
 *		void *arg
 *
 *	The "arg" may point to an auxillary data structure needed by
 *	the comparison function.
 *
 *	The compare function shall return an integer indicating their ordering:
 *		if ret < 0 then col1 sorts before col2
 *		if ret == 0 then col1 sorts as equal to col2
 *		if ret > 0 then col1 sorts after col2
 *
 *	Example use:
 *
 *	int mytype_compare(void *col1, void *col2, int l1, int l2, void *arg);
 *	...
 *	nsort_declare_function("mytype", mytype_compare, mytype_arg);
 *	...
 *	nsort_define("-field:name=fld1,size=4,off=0,compare=mytype"
 *		     "-key:mytype", 0, NULL, &context);
 *
 */
nsort_msg_t nsort_declare_function(char *name, nsort_compare_t func, void *arg);

/* nsort_version - get a string that contains the nsort library version number
 */
char	*nsort_version(void);

/* nsort_raise_error - routine that can be used (only) by a user-defined
 *			comparison routine or a merge input callback routine
 *			to raise an error.
 * Arguments:
 *	err_msg -	error message string that will be incorporated as
 *			part of the nsort library's error message.
 *
 *	This routine will not return unless it fails, in which case it will
 *	return a string that indicates why it failed.
 */
char *nsort_raise_error(char *err_msg);

/* nsort_get_thread_index - routine that can be used by comparison functions
 *			to get the nsort-context-specific index of the nsort 
 *			thread calling the comparison function.  
 * Arguments:
 *	None
 *
 *	Returns: 0 if the thread is the nsort main thread.
 *               1 - N if the thread is one of the N nsort "sort" threads.
 *               -1 if the thread cannot be determined.
 */
int nsort_get_thread_index(void);

/* Macros for mapping other sort API calls to Nsort API calls.
 */	
#if !defined(SYNC_SUCCESS)
# define SYNC_SUCCESS		NSORT_SUCCESS
# define SYNC_END_OF_OUTPUT	NSORT_END_OF_OUTPUT
# define SYNC_ERROR		(-1)

# define sync_setup(def, len, a, b, c, d, e, f, g, ctxp)	nsort_define((def), 0, NULL, (ctxp))
# define sync_release_record(buf, size, ctxp) nsort_release_recs((buf), *(size), (ctxp))
# define sync_release_end(ctxp)	 nsort_release_end(ctxp)
# define sync_return_record(buf, sizep, ctxp) nsort_return_recs((buf), (sizep), (ctxp))
# define sync_end(ctxp)	 	nsort_end(ctxp)
#endif

#ifdef __cplusplus
}
#endif

#if defined(__sparc) && defined(__sun)
# if !defined(_REENTRANT) && !defined(NSORT_SKIP_REENTRANT_CHECK)
#  error "_REENTRANT not defined -- cc -mt needed?"
# endif
#endif

#endif /* !_NSORT_H_ */
