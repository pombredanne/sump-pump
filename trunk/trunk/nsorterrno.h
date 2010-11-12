/*
 *    Copyright 1996-2007  Ordinal Technology Corp.  All Rights Reserved.
 *
 *	Nsorterrno.h	- Error and warning codes for the nsort subroutine library
 */
#ifndef _NSORTERRNO_H_
#define _NSORTERRNO_H_

#define NSORT_SUCCESS			0

#define	NSORT_FIRST_WARNING	1
#define	NSORT_FIRST_INFO	10000

#define NSORT_MSG_IS_ERROR(msg)		((msg) < 0)
#define NSORT_MSG_IS_WARNING(msg)	((msg) >= NSORT_FIRST_WARNING && (msg) < NSORT_FIRST_INFO)
#define NSORT_MSG_IS_INFO(msg)		((msg) >= NSORT_FIRST_INFO)


/* nogo errors: the sort will not execute */
#define NSORT_STATEMENT_START		(-1)
#define NSORT_COLON_EXPECTED		(-2)
#define NSORT_COMMA_EXPECTED		(-3)
#define NSORT_UNEXPECTED_IN_LIST	(-4)
#define NSORT_PAREN_EXPECTED_BEFORE	(-5)
#define NSORT_PAREN_EXPECTED_AFTER	(-6)
#define NSORT_AMBIGUOUS_IDENTIFIER	(-7)
#define NSORT_UNEXPECTED_KEYWORD	(-8)
#define NSORT_UNKNOWN_KEYWORD		(-9)
#define NSORT_IO_ERROR			(-10)
#define NSORT_NEEDS_POSITIVE_INT	(-11)
#define NSORT_SCALE_OVERFLOW		(-12)
#define NSORT_TYPE_MISMATCH		(-13)
#define NSORT_TYPE_WITHOUT_SIZE		(-14)	/* This type needs a size:<number> specification */
#define NSORT_EXTENDS_PAST_END		(-15)	/* position + size > record len (parse time)*/
#define NSORT_UNSIGNED_WITHOUT_TYPE	(-16)	/* The 'unsigned' qualifier is supported only for integer types */
#define NSORT_FIELD_NEEDS_EQ		(-17)	/* "name" expects "=" or ":" */
#define NSORT_METHOD_NEEDS_EQ		(-18)
#define NSORT_SPECIFICATION_NEEDS_EQ	(-19)
#define NSORT_FIELD_ALREADY_NAMED	(-20)	/* a second NAME=xxx for /field */
#define NSORT_BAD_FIELD_NAME		(-21)	/* no ident found where field name expected */
#define NSORT_BAD_REC_SIZE		(-22)	/* record_size: < 1 or > 65534 no good */
#define NSORT_BAD_REC_SIZE_SPEC		(-23)	/* record_size: needs an int or 'variable' */
#define NSORT_REC_MUST_BE_VARLEN	(-24)	/* max,lrl, or min rec len needs varlen recs */
#define NSORT_MAXLEN_NEEDS_INT		(-25)	/* max or needs :<integer> */
#define NSORT_MAXLEN_INVALID		(-26)	/* max or <1 or > 65534 */
#define NSORT_MINLEN_NEEDS_INT		(-27)	/* min needs :<integer> */
#define NSORT_MINLEN_INVALID		(-28)	/* min < 1 or > 65534 */
#define NSORT_CHARACTER_NEEDED		(-29)	/* A character constant (e.g. ',') is expected here */
#define NSORT_TYPE_NOT_DELIMITABLE	(-30)	/* delimiter for non-string type */
#define NSORT_FIELD_ALREADY_TYPED	(-31)	/* field already has a type */
#define NSORT_FLOAT_SIZE		(-32)	/* float must be size 4 or unspecified */
#define NSORT_DOUBLE_SIZE		(-33)	/* double must be size 8 or unspecified */
#define NSORT_PACKED_TOO_LONG		(-34)	/* packed ! in [1.. MAXPACKEDSIZE] */
#define NSORT_FIELD_TOO_SHORT		(-35)	/* The ending position of a field must be after the start position */
#define NSORT_POSITION_NEEDED		(-36)	/* INTVALUE expected after position: */
#define NSORT_DUPLICATE_FIELDNAME	(-37)	/* another field already has this name */
#define NSORT_FIELD_SIZE_NEEDED		(-38)	/* INTVALUE expected after size: */
#define NSORT_NO_LICENSE		(-39)	/* A license is required to use Nsort */
#define NSORT_FIELD_ALREADY_SIZED	(-40)	/* field already has a size specification */
#define NSORT_FIELD_ALREADY_DELIMITED	(-41)	/* field already has a delimiter */
#define NSORT_FIELD_SYNTAX		(-42)	/* unknown token inside /field (...) list */
#define NSORT_POSITION_POSITIVE		(-43)	/* position:N but N was zero */
#define NSORT_KEY_FIELD_MISSING		(-44)	/* The field \"%s\" in this key specification is undefined */
#define NSORT_KEY_ALREADY_TYPED		(-45)	/* two type definitions in a key */
#define NSORT_MMAP_ZERO_FAILED		(-46)	/* mmap %d bytes of /dev/zero failed: %s */
#define NSORT_REDUNDANT_ORDERING	(-47)
#define NSORT_KEY_SYNTAX		(-48)
#define NSORT_KEY_NUMBER		(-49)
#define NSORT_KEY_NUMBER_INVALID	(-50)	/* number:n < 1 or > 255 */
#define NSORT_KEY_NUMBER_DUPLICATE	(-51)	/* Number:n when another key already has n */
#define NSORT_DERIVED_NEEDS_VALUE	(-52)
#define NSORT_FIELD_MISSING		(-53)	/* The field %s is undefined */
#define NSORT_NON_NUMERIC_DERIVED	(-54)
#define NSORT_NON_STRING_DERIVED	(-55)
#define NSORT_PAGESIZE_POW2		(-56)	/* pagesize isn't a power of 2*/
#define NSORT_SUMMARIZE_FIELD_MISSING	(-57)	/* (as yet) unspecified field in /summarize stmt */
#define NSORT_OPEN_FAILED		(-58)	/* The file %s was not opened: %s */
#define NSORT_MEMORY_NEEDED		(-59)	/* /memory= without an integer # of kbytes */
#define NSORT_IOSIZE_UNALIGNED		(-60)	/* {in,out,temp}file buf=N % dio.d_miniosz */
#define NSORT_IOSIZE_TOO_LARGE		(-61)	/* {in,out,temp}file buf=N > dio.d_maxiosz */
#define NSORT_LICENSE_FAILURE		(-62)	/* %s: %s */
#define NSORT_LICENSE_MALFORMED		(-63)	/* Missing license field ... */
#define NSORT_CPU_REQ_NEEDED		(-64)	/* /processors= without an integer # of cpus */
#define NSORT_ONLY_FIXED_EDITING	(-65)	/* Nsort supports editing of fixed size records only */
#define NSORT_CAT_ONLY_ONE		(-66)	/* Multiple output files are not supported for file copying and concatenation */
#define NSORT_FORMAT_SPEC		(-67)	/* A record format specification needs either size:N or delimiter:C */
#define NSORT_BAD_METHOD		(-68)
#define NSORT_BAD_HASHSPEC		(-69)
#define NSORT_VARIABLE_NEEDS_KEY	(-70)
#define NSORT_BAD_CHARACTER_SPEC	(-71)	/* bad escaped character e.g. '\0q' */
#define NSORT_CHARACTER_TOO_LARGE	(-72)	/* oversized character e.g. '\06777' */
#define NSORT_MISSING_QUOTE		(-73)	/* no trailing single quote in a CHARVALUE */
#define NSORT_FILENAME_MISSING		(-74)	/* zero-length filename (e.g. "tempfil=," ) */
#define NSORT_FILESYS_NAME_MISSING	(-75)	/* zero-length filename/filesysname */
#define NSORT_SUMMARIZE_DUPLICATES	(-76)	/* asked to do keep-dups summarizing sort */
#define NSORT_NOT_STATEMENT		(-77)	/* stmt starts with a non-command token */
#define NSORT_EXTRA_INSIDE_STATEMENT	(-78)	/* spurious chars inside statement's comma-list */
#define NSORT_EXTRA_AFTER_STATEMENT	(-79)	/* spurious chars after apparent end-of-statement */

#define NSORT_EXTRA_REFORMAT		(-80)	/* This is an extra reformat statement */
#define NSORT_RECORD_SORTS_VARLEN	(-81)	/* explicit record sort directive for varlen */
#define NSORT_RECORD_SORT_TOO_LARGE	(-82)	/* explicit record sort directive for big rec */
#define NSORT_RECORD_TOO_LONG		(-83)	/* varlen: rec longer than decl'd max rec len */
#define NSORT_RECORD_TOO_SHORT		(-84)	/* varlen: rec shorter than decl'd min reclen */
#define NSORT_REFORMAT_TOO_FAR		(-85)	/* Reformatting at the field "%s" would exceed the maximum record size of %s */
#define NSORT_PARTIAL_RECORD		(-86)	/* Data format error: the size of %s is not a multiple of the record size */
#define NSORT_DELIM_MISSING		(-87)	/* The record at %d in file %d had no record delimiter */
#define NSORT_RADIX_PREHASH_INCOMPAT	(-88)	/* -meth:radix,prehash not supported together */
#define NSORT_FIELD_BEYOND_END		(-89)	/* field extends past eorec (runtime) */
#define NSORT_PAST_MEMORY_LIMIT		(-90)	/* a /memory request beyond rlimit(RSS) */
#define NSORT_SPEC_OPEN			(-91)	/* Specification file %s open failure %s */
#define NSORT_SPEC_TOO_DEEP		(-92)	/* Loop detected: /specification file %s including %s */
#define NSORT_INPUT_OPEN		(-93)	/* Input file %s open failure %s */
#define NSORT_OUTPUT_OPEN	(-94)	/* Output file %s open failure %s */
#define NSORT_FILESYS_ERRNO	(-95)	/* stat{,vfs} failure on a file/filesys name */
#define NSORT_TEMPFILE_STAT	(-96)	/* Work_file %s stat failure %s */
#define NSORT_TEMPFILE_BAD_TYPE	(-97)	/* Work_file %s is not a dir or regular file */
#define NSORT_TEMPFILE_OPEN	(-98)	/* Work_file %s open failure %s */
#define NSORT_MAX_LT_MIN	(-99)	/* The max i/o for -configure may not be less than the min i/o size */
#define NSORT_FIELD_MAX_ONLY_SEP (-100) /* Only separated fields may have maximum_size specifications */
#define NSORT_HASH_WITHOUT_RADIX (-101)	/* Hashing is supported only in radix sorts */
#define NSORT_MEMORY_TOO_SMALL	(-102)	/* This sort requires at least %dM of memory */
#define NSORT_INVALID_CONTEXT	(-103)	/* invalid context passed to an api function */
#define NSORT_INVALID_ARGUMENT	(-104)	/* invalid argument to an api function */
#define NSORT_FIELD_EXCEEDS_MAX	(-105) /* The field \"%s\" at %lld in \"%s\" size %d exceeds the maximum size %d */
#define NSORT_MALLOC_FAIL	(-106)	/* malloc returned null */
#define NSORT_APIFILES		(-107)	/* api sort has input or output file specification */
#define NSORT_INTERNAL_NO_MEM	(-108)	/* internal sort requires more mem than is available */
#define NSORT_KEY_BEYOND_END	(-109)	/* key extends past eorec (runtime) */
#define NSORT_ERROR_IGNORED	(-110)	/* This sort died due to a fatal err */
#define NSORT_CANCELLED		(-111)	/* sort cancelled; no more ops allowed*/
#define NSORT_CANT_SEEK		(-112)	/* The file %s is not seekable and cannot be used for %s i/o */
#define NSORT_INT_SIZE		(-113)	/* The size of a signed integer may be 1, 2, 4, or 8 bytes */
#define NSORT_SUMMARY_NEEDS_INT (-114)	/* A summarizing field must be a 4 or 8 byte integer */
#define NSORT_POSITION_REQUIRED (-115)	/* A position:N qualification is needed */
#define NSORT_SYNTAX_ERROR	(-116)	/* Syntax error */
#define NSORT_EXTRA_HEX_DIGIT	(-117)	/* Hexadecimal strings must contain an even number of bytes */
#define NSORT_BAD_HEX_DIGIT	(-118)	/* Hexadecimal strings may contain characters only in the range [0-9A-Fa-f] */
#define NSORT_STRING_TOO_LONG	(-119)	/* This string is too long; the limit is %d bytes */
#define NSORT_MISSING_DOUBLE_QUOTE (-120) /* Trailing double quote (") missing*/
#define NSORT_TOO_MANY_KEYS	(-121) /* Too many keys were defined: limit is at least %d */
#define NSORT_SUMMARIZED_KEY	(-122) /* The summary field %s overlaps a key */
#define NSORT_MERGE_MISORDERED	(-123) /* Record #%lld at %lld in file \"%s\" is misordered" */
#define NSORT_MERGE_NOEDIT	(-124) /* Record editing is not yet supported for -merge */
#define NSORT_APPEND_CHANGED	(-125) /* Output file \"%s\" changed during sort, append cancelled */
#define NSORT_APPEND_NOSTDOUT	(-126) /* Append to standard output is not supported */
#define NSORT_MLD_CREATE	(-127)	/* The creation of a memory locality domain failed: %s */
#define NSORT_MLDSET_CREATE	(-128)	/* The creation of the memory locality domain set failed: %s */
#define NSORT_MLDSET_PLACE	(-129)	/* The placement of the memory locality domain set failed: %s */
#define NSORT_TERM_SYNTAX	(-130)	/* Syntax error in boolean term */
#define NSORT_RE_RANGE_END	(-131)	/* range endpoint too large. */
#define NSORT_RE_BAD_NUMBER	(-132)	/* bad number */
#define NSORT_RE_DIGIT_RANGE	(-133)	/* digit out of range */
#define NSORT_RE_DELIMITER	(-134)	/* illegal or missing delimiter */
#define NSORT_RE_REMEMBERED	(-135)	/* no remembered search string */
#define NSORT_RE_PAREN_IMBALANCE (-136)	/* \( \) imbalance */
#define NSORT_RE_TOO_MANY_PARENS (-137)	/* too many \( */
#define NSORT_RE_TOO_MANY_NUMS	(-138)	/* more than 2 numbers given in \{ \} */
#define NSORT_RE_BRACE_EXPECTED (-139)	/* } expected after \ */
#define NSORT_RE_NUM_PAIR	(-140)	/* number exceeds second in \{ \} */
#define NSORT_RE_BKT_IMBALANCE	(-141)	/* [ ] imbalance */
#define NSORT_RE_EXPBUF_OVERFLOW (-142)	/* regular expression overflow */
#define NSORT_GENERATE_COUNT	(-143)	/* A record count must be specified with -generate */
#define NSORT_REFORMAT_FIELD_MISSING	(-144)	/* The field %s is not available for reformatting */
#define NSORT_EXPECTED_THEN	(-145)	/* The keyword "then" was expected here */
#define NSORT_FIELD_REMOVED	(-146)	/* The field \"%s\" was removed by /reformat */
#define NSORT_EXPECTED_ELSE	(-147)	/* The keyword "else" was expected here */
#define NSORT_TYPE_CHANGED	(-148) /* "Reformat changed types incompatibly %s to %s" */
#define NSORT_REFORMAT_KEY_MISSING (-149) /* The key %s was removed by reformat */
#define NSORT_EXCESSIVE_CONSTANT (-150)	/* The type %s is not large enough to conain %lld */
#define NSORT_RECURSIVE_DERIVED	(-151)	/* The value of "%s" may not refer to itself */
#define NSORT_REFORMAT_FIELD_BOOL (-152) /* The condition \"%s\" is not available for reformatting */
#define NSORT_MONTH_TOO_SHORT	(-153)	/* A field of type \"month\" must be at least 3 characters long */
#define NSORT_FILTER_INCOMPAT_KEYS (-154) /* A filter or copy may not have any keys */
#define NSORT_INCOMPAT_RECORD_TYPE (-155) /* Separated fields are only supported in separated records */

#define NSORT_PADTYPES_DIFFER	(-156) /* Two strings have different pad chracters: 0x%02x 0x%02x in %s */
#define NSORT_RECORD_MTBUF_SIZE	(-157)	/* varlen: rec longer than calculated temporary block size */
#define NSORT_UNSUPPORTED_MERGE_SELECTOR (-158) /* Merge does not support input selection */
#define NSORT_MISSPELLED_KEYWORD (-159) /* Unexpected token; perhaps a misspelled command name? */
#define NSORT_STRING_EXPECTED	(-160)	/* A quoted string is needed here */
#define NSORT_FILTER_CANNOT_GROW (-161) /* Record copying currently does not allow records to become larger */
#define NSORT_UNUSED_REFORMAT	(-162)	/* Record copying ignores input reformatting when output reformatting is also specified */
#define NSORT_BAD_DELIM_MOD	(-163)	/* Unrecognized field modifier; valid ones are bdfiMnr */
#define NSORT_COL_DOT_OFF	(-164)	/* A delimited field is specified as <column_number>[.<character_offset] */
#define NSORT_IMPLICIT_DERIVED_REFORMAT	(-165)	/* Derived fields must be explicitly added to non fixed-size records */
#define NSORT_BAD_FIELD_SIZE	(-166)	/* Field sizes may range from 1 to 65535 */
#define NSORT_REFORMAT_KEY_UNNAMED (-167) /* A key must refer to a named field when reformatting the input record */
#define NSORT_REFORMAT_FIXED_STREAM (-168)	/* Stream records do not support reformatting of fixed sized fields such as \"%s\" */
#define NSORT_DELIMFIELDS_DEPRECATED (-169)	/* Delimited fields are no longer supported */
#define NSORT_FIELD_ALREADY_POSITIONED (-170)	/* This field has already been positioned */
#define NSORT_REFORMAT_DELIMITED	(-171)	/* The delimiteed field %s may not be reformatted */
#define NSORT_KEY_EXCEEDS_MAX		(-172) /* The key \"%s\" at %lld in \"%s\" size %d exceeds the maximum size %d */
#define NSORT_REMAINDER_REFORMAT	(-173)	/* The field \"%s\" contains an unknown number of subfields and may be placed only at the end of the reformat list */
#define NSORT_BINARY_IN_DELIMITED	(-174)	/* Delimited records cannot contain fields of type %2$s */
#define NSORT_SUMMARIZE_NEEDS_KEY	(-175)	/* No key was given for a summarizing sort */
#define NSORT_LOGICAL_EXPR_NEEDED	(-176)  /* This must be a logical expression */
#define NSORT_UNSUPP_CHANGE_RECORD	(-177)	/* Nsort does not yet support changing between fixed-size and stream records */
#define NSORT_CANNOT_DETERMINE_POSITION (-178)	/* (NSORT_SUMMARIZE_NEEDS_KEY - 1) */
#define NSORT_OUT_OF_SWAP		(-179)	/* Insufficient swap space for %d of memory: %strerror */
#define NSORT_RESOURCE_LIMITED		(-180)	/* Resource limits constrained Nsort to only %dMB of memory */
#define NSORT_32BIT_LIMITED		(-181)	/* This 32-bit Nsort is constrained to use at most %dMB of memory */
#define NSORT_SYSCALL			(-182)	/* API call failed: %s %strerror */
#define NSORT_INVALID_PHASE		(-183)	/* API call out of sequence */
#define NSORT_LOCK_FAILED		(-184)	/* API call: mutex_lock failed; %s */
#define NSORT_UNIMPLEMENTED		(-185)	/* feature %s not yet implemented */
#define NSORT_UNLOCK_FAILED		(-186)	/* API call: mutex_unlock failed; %s */
#define NSORT_COMPARE_UNDECLARED	(-187)	/* The comparison function %s has not been declared */
#define NSORT_FOLD_POINTER		(-188)	/* The 'fold_upper' and 'fold_lower' modifiers are currently supported only for pointer sorts */
#define NSORT_THREAD_CREATE		(-189)	/* Nsort could not create a thread: %s */
#define NSORT_PWRITE64_ERROR		(-190)	/* pwrite64 system call does not work on this Solaris 2.7 server, please install Solaris patch 106980-18 */
#define NSORT_BAD_MERGE_INPUT		(-191)  /* The merge input callback function is not defined */
#define NSORT_BAD_MERGE_WIDTH		(-192)  /* The merge width has not been specified */
#define NSORT_API_DEFINE_MERGE		(-193)  /* Merge is not allowed with nsort_define(), use nsort_merge_define() instead */
#define NSORT_API_MERGE_INPUT		(-194)  /* Input file definitions are not allowed with nsort_merge_define() */
#define NSORT_UNKNOWN_SCALE		(-195)  /* Invalid numeric scale in \"%s\"; supported values are [kmgt] */
#define NSORT_NUMBER_OVERFLOW		(-196)  /* The number in \"%s\" is too large for a 64-bit integer */
#define NSORT_NEEDS_STRING		(-197)  /* A character string is needed here */
#define NSORT_EXTRA_DOES		(-198)  /* Syntax error: this operator already has a 'does' modifier */
#define NSORT_EXTRA_NOT			(-199)  /* Syntax error: this operator already has 'not' modifier */
#define NSORT_NOT_UNSUPPORTED		(-200)  /* Syntax error: this operator does not support the 'not' modifier */
#define NSORT_IN_REC_SIZE_REQUIRED	(-201)  /* The input format statement needs an integer record size */
#define NSORT_BAD_BIT_FIELD		(-202)  /* Illegal bit field declaration */
#define NSORT_BIT_POSITION_TOO_LARGE	(-203)  /* Bit field offsets can range from 0 through 7 */
#define NSORT_BIT_SIZE_TOO_LARGE	(-204)  /* Bit field sizes cannot exceed their position: */
#define NSORT_REFORMAT_NEEDS_MAXSIZE	(-205)  /* Please specify a maximum size for the variable string field "%s" */
#define NSORT_VALUE_CONTAINS_SPECIAL	(-206)	/* The "%s" value "%s" may not contain the %s */
#define NSORT_PARTIAL_VARIABLE_RECORD	(-207)	/* The last record in \"%s\" (offset %s) would extend beyond the end of the file" */
#define NSORT_EXTRA_FORMAT		(-208)	/* The format of this file has already been specified */
#define NSORT_SIZE_BEYOND_LICENSE	(-209)	/* This sort is larger than the license  */
#define NSORT_PACKED_SIZE_TOO_LARGE	(-210)	/* "The size of a packed decimal field cannot exceed 31 */
#define NSORT_PACKED_OVERFLOW		(-211)  /* The number in \"%s\" is too large for a packed or zone decimal number */
#define NSORT_MERGE_CALLBACK_OVERWRITE	(-212)	/* The merge input callback function for input stream %s offset %s size %s wrote beyond the end of the buffer */
#define NSORT_USER_RAISED_ERROR		(-213)	/* User-defined comparison or merge input callback error: %s */
#define NSORT_STATFILE_STAT	(-214)	/* Statistics file %s could not be opened: %s */
#define NSORT_STATFILE_BAD_TYPE	(-215)	/* Statistics file %s is not a dir or regular file */
#define NSORT_STATFILE_OPEN	(-216)	/* Statistics file %s could not be opened for writing: %s */
#define NSORT_KEY_DIFF_CONFLICT	(-217)	/* Prepending the key difference is compatible only with delimited records and not with multiple output files, output record selection or reformatting */


/*
 * These non-fatal warnings return have positive values
 */
#define NSORT_END_OF_OUTPUT	1

#define NSORT_CLOSE_FAILED	2	/* The file %s was not closed: %s */

#define NSORT_UNLINK_FAILED	3	/* The temp file %s could not be removed: %s */
#define NSORT_MEMORY_MINIMAL	4	/* operation too big for nsort's default memory allocation, may cause paging */
#define NSORT_EXCESSIVE_PAGING	5	/* Performance caution: Excessive paging (%d faults) detected  */
#define NSORT_REDUCING_IOSIZE	6	/* The default i/o size of %d is too large for this merge; reducing to %d */
#define NSORT_PMTRACE_PROBLEM	7	/* Peformance Co-Pilot pmdatrace error for %s: %s */
#define NSORT_SUMMARIZE_OVERFLOWED 8	/* Some summarizations would have overflowed; the output may contain up to %s duplicate keys */
#define NSORT_CPU_SUBSET_LIMIT	9	/* The %s license has a limit of %s processors */
#define NSORT_CPU_REQ_TOOBIG	10	/* #cpus asked for > #cpus in this system */
#define NSORT_CPU_REQ_RESTRICTED 11	/* #cpus asked for > #unrestricted cpus in sys*/
#define NSORT_RECSORT_OUTFILES	12	/* Multiple output files require method:pointer; continuing */
#define NSORT_POINTER_SORT_ONLY	13	/* Output file editing and selection require method:pointer; continuing */
#define NSORT_TEMPFS_INAPPROPRIATE 14	/* %.*s is on a tmpfs filesystem; this may cause poor performance */
#define NSORT_CONVERSIONS_OVERFLOWED 15	/* The values of %d expressions were too large to fit in their derived fields */
#define NSORT_MEMORY_REDUCED	16	/* (win) System memory resources are limited; memory use reduced from %sB to %sB */
/* #define NSORT_SPAREWARN2	17	spare */
#define NSORT_MLDLINK		18	/* Process_mldlink() failed in sproc %d: %s */
#define NSORT_RUNANYWHERE	19	/* Sysmp(RUNANYWHERE) failed in sproc %d: %s */
#define NSORT_UNDEFINED_KEY	20	/* The semantics for the key %s are not defined */
#define NSORT_KEY_TOO_SHORT	21	/* The key %s starts after it ends */
#define NSORT_DETAIL_IO		22	/* Error trying to %s detailed statistics log \"%s\" at %lld bytes: %s; continuing without detail log */
#define NSORT_RETURN_BUF_SMALL	23	/* The return buffer is too small (size %d) to hold the next result record (size %d) */
#define NSORT_IGNORING_KEY	24	/* The key %s is the same as %s; ignoring it */
#define NSORT_DELIMITER_ADDED	25	/* The file \"%s\" did not end with the record delimiter; nsort has added one */
#define NSORT_PREALLOCATE	26	/* Preallocation failure \"%s\": %s */
#define NSORT_NO_CLIENT_INPUT	27	/* API ERROR: nsort_release_recs() called while reading from an external file */
#define NSORT_NO_CLIENT_OUTPUT	28	/* API ERROR: nsort_return_recs() called while writing to an external file */

#endif /* _NSORTERRNO_H_ */
