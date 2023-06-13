/*
 * AIFF format header file
 * bytes are stored in 68000 = big endian order
 */

/*
 * some of the audio parameters in an AIFF file
 */
typedef struct
{
    int  samprate;
    int  nchannels;
    int  sampwidth;
} audio_params_t;

/*
 * all chunks consist of a chunk header followed by some data
 *
 * WARNING: the spec says that every chunk must contain an even number
 * of bytes. A chunk which contains an add number of bytes is padded with
 * a trailing zero byte which is NOT counted in the chunk header's size
 * field.
 */
typedef struct
{
    char id[4];
    int  size;
} chunk_header_t;

#define CHUNK_ID     4
#define CHUNK_HEADER 8

typedef struct
{
    chunk_header_t header;
    int file_position;  /* not in AIFF file */
    char type[4]; /* should contain 'AIFF' for any audio IFF file */
} form_chunk_t;

#define FORM_CHUNK      12  /* including the header */
#define FORM_CHUNK_DATA 4

#define COMM_CHUNK      26   /* including the header */
#define COMM_CHUNK_DATA 18

typedef struct
{
    chunk_header_t header;
    int file_position;            /* not in AIFF file */
    short nchannels;
    unsigned int  nsampframes;
    short sampwidth;
    int  samprate;              /* not in AIFF file */
} comm_chunk_t;


#define SSND_CHUNK      16   /* including the header */
#define SSND_CHUNK_DATA 8

typedef struct
{
    chunk_header_t header;
    unsigned int  offset;
    unsigned int  blocksize;

    int  file_position; /* not in AIFF file */
    int  sample_area_bytes; /* not in AIFF file */
} ssnd_chunk_t;
