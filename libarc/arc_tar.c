#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <ctype.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "mblock.h"
#include "zip.h"
#include "arc.h"

#define TARBLKSIZ 512
#define TARHDRSIZ 512

static long octal_value(char *s, int len);
static int tar_checksum(char *hdr);

ArchiveEntryNode *next_tar_entry(ArchiveHandler archiver)
{
    char hdr[TARHDRSIZ];
    long size, sizeb;
    ArchiveEntryNode *entry;
    URL url;
    int flen;
    int macbin_check;

    url = archiver->decode_stream;
    macbin_check = (archiver->nfiles == 0);

  retry_read:
    if(url_read(url, hdr, TARHDRSIZ) != TARHDRSIZ)
	return NULL;
    if(macbin_check && hdr[0] == 0 && hdr[128] != 0)
    {
	int i;

	macbin_check = 0;
	if(memcmp(hdr + 128 + 257, "ustar", 5) != 0)
	    return NULL;
	for(i = 0; i < TARHDRSIZ - 128; i++)
	    hdr[i] = hdr[i + 128];
	if(url_read(url, hdr + TARHDRSIZ - 128, 128) != 128)
	    return NULL;
	if(archiver->type == AHANDLER_SEEK)
	    archiver->pos += 128;
    }
    else
    {
	if(hdr[0] == '\0')
	    return NULL;
	if(!tar_checksum(hdr))
	    return NULL;
    }
    size = octal_value(hdr + 124, 12);
    flen = strlen(hdr);
    if(size == 0 && flen > 0 && hdr[flen - 1] == '/')
    {
	if(archiver->type == AHANDLER_SEEK)
	    archiver->pos += TARHDRSIZ;
	goto retry_read;
    }

    entry = new_entry_node(&archiver->pool, hdr, flen);
    if(entry == NULL)
	return NULL;
    sizeb = (((size) + (TARBLKSIZ-1)) & ~(TARBLKSIZ-1));

    if(archiver->type == AHANDLER_SEEK)
    {
	archiver->pos += TARHDRSIZ;
	entry->strmtype = ARCSTRM_SEEK_URL;
	entry->comptype = ARCHIVEC_STORED;
	entry->compsize = entry->origsize = size;
	entry->u.seek_start = archiver->pos;
	url_skip(url, sizeb);
	archiver->pos += sizeb;
    }
    else
    {
	DeflateHandler encoder;
	char buff[BUFSIZ];
	long compsize, n;

	archiver->pos = entry->origsize = size;
	compsize = 0;
	encoder = open_deflate_handler(archiver_read_func, archiver,
				       ARC_DEFLATE_LEVEL);
	while((n = zip_deflate(encoder, buff, sizeof(buff))) > 0)
	{
	    push_memb(&entry->u.compdata, buff, n);
	    compsize += n;
	}
	close_deflate_handler(encoder);
	entry->strmtype = ARCSTRM_MEMBUFFER;
	entry->comptype = ARCHIVEC_DEFLATED;
	entry->compsize = compsize;
	url_skip(url, sizeb - size);
    }

    return entry;
}

static long octal_value(char *s, int len)
{
    long val;

    while(len > 0 && !isdigit((int)(unsigned char)*s))
    {
	s++;
	len--;
    }

    val = 0;
    while(len > 0 && isdigit((int)(unsigned char)*s))
    {
	val = ((val<<3) | (*s - '0'));
	s++;
	len--;
    }
    return val;
}

static int tar_checksum(char *hdr)
{
    int i;

    long recorded_sum;
    long unsigned_sum;		/* the POSIX one :-) */
    long signed_sum;		/* the Sun one :-( */

    recorded_sum = octal_value(hdr + 148, 8);
    unsigned_sum = 0;
    signed_sum = 0;
    for(i = 0; i < TARBLKSIZ; i++)
    {
	unsigned_sum += 0xFF & hdr[i];
	signed_sum   += hdr[i];
    }

    /* Adjust checksum to count the "chksum" field as blanks.  */
    for(i = 0; i < 8; i++)
    {
	unsigned_sum -= 0xFF & hdr[148 + i];
	signed_sum   -= hdr[i];
    }
    unsigned_sum += ' ' * 8;
    signed_sum   += ' ' * 8;

    return unsigned_sum == recorded_sum || signed_sum == recorded_sum;
}
