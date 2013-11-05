/*
--------------------------------------------------------------------------------
-   Module      :   mem_file.c
-   Description :   A general purpose library for manipulating a memory area
-                   as if it were a file.
-                   mfs_ stands for memory file system.
-   Author      :   Mike Johnson - Banctec AB 03/07/96
-                   
--------------------------------------------------------------------------------
*/

/* 

Copyright (c) 1996 Mike Johnson
Copyright (c) 1996 BancTec AB

Permission to use, copy, modify, distribute, and sell this software
for any purpose is hereby granted without fee, provided
that (i) the above copyright notices and this permission notice appear in
all copies of the software and related documentation, and (ii) the names of
Mike Johnson and BancTec may not be used in any advertising or
publicity relating to the software.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  

IN NO EVENT SHALL MIKE JOHNSON OR BANCTEC BE LIABLE FOR
ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
OF THIS SOFTWARE.

*/


/*
--------------------------------------------------------------------------------
-   Includes
--------------------------------------------------------------------------------
*/

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

/*
--------------------------------------------------------------------------------
-   Definitions
--------------------------------------------------------------------------------
*/

#define MAX_BUFFS   20
#define FALSE       0
#define TRUE        1

/*
--------------------------------------------------------------------------------
-   Globals
--------------------------------------------------------------------------------
*/

static char *buf[MAX_BUFFS];        /* Memory for each open buf */
static long  buf_off[MAX_BUFFS];    /* File pointer for each buf */
static long  buf_size[MAX_BUFFS];   /* Count of bytes allocated for each buf */
static long  fds[MAX_BUFFS];        /* File descriptor status */
static int   buf_mode[MAX_BUFFS];   /* Mode of buffer (r, w, a) */

static int library_init_done = FALSE;


/*
--------------------------------------------------------------------------------
-   Function prototypes
--------------------------------------------------------------------------------
*/

int mfs_open (void *ptr, int size, char *mode);
int mfs_lseek (int fd, int offset, int whence);
int mfs_read (int fd, void *buf, int size);
int mfs_write (int fd, void *buf, int size);
int mfs_size (int fd);
int mfs_map (int fd, char **addr, size_t *len);
int mfs_unmap (int fd);
int mfs_close (int fd);
static int extend_mem_file (int fd, int size);
static void mem_init ();

/*
--------------------------------------------------------------------------------
-   Function code
--------------------------------------------------------------------------------
*/

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_open ()
-
-   Arguments   :   Pointer to allocated buffer, initial size of buffer, 
-                   mode spec (r, w, a)
-
-   Returns     :   File descriptor or -1 if error.
-
-   Description :   Register this area of memory (which has been allocated
-                   and has a file read into it) under the mem_file library.
-                   A file descriptor is returned which can the be passed
-                   back to TIFFClientOpen and used as if it was a disk
-                   based fd.
-                   If the call is for mode 'w' then pass (void *)NULL as
-                   the buffer and zero size and the library will 
-                   allocate memory for you.
-                   If the mode is append then pass (void *)NULL and size
-                   zero or with a valid address.
-                   
--------------------------------------------------------------------------------
*/

int mfs_open (void *buffer, int size, char *mode)
{
    int ret, i;
    void *tmp;

    if (library_init_done == FALSE)
    {
        mem_init ();
        library_init_done = TRUE;
    }

    ret = -1;

    /* Find a free fd */

    for (i = 0; i < MAX_BUFFS; i++)
    {
        if (fds[i] == -1)
        {
            ret = i;
            break;
        }
    }

    if (i == MAX_BUFFS)     /* No more free descriptors */
    {
        ret = -1;
        errno = EMFILE;
    }

    if (ret >= 0 && *mode == 'r')
    {
        if (buffer == (void *)NULL)
        {
            ret = -1;
            errno = EINVAL;
        }
        else
        {
            buf[ret] = (char *)buffer;
            buf_size[ret] = size;
            buf_off[ret] = 0;
        }
    }
    else if (ret >= 0 && *mode == 'w')
    {

        if (buffer != (void *)NULL)
        {
            ret = -1;
            errno = EINVAL;
        }

        else
        {
            tmp = malloc (0);   /* Get a pointer */
            if (tmp == (void *)NULL)
            {
                ret = -1;
                errno = EDQUOT;
            }
            else
            {
                buf[ret] = (char *)tmp;
                buf_size[ret] = 0;
                buf_off[ret] = 0;
            }
        }
    }
    else if (ret >= 0 && *mode == 'a')
    {
        if (buffer == (void *) NULL)    /* Create space for client */
        {
            tmp = malloc (0);   /* Get a pointer */
            if (tmp == (void *)NULL)
            {
                ret = -1;
                errno = EDQUOT;
            }
            else
            {
                buf[ret] = (char *)tmp;
                buf_size[ret] = 0;
                buf_off[ret] = 0;
            }
        }
        else                            /* Client has file read in already */
        {
            buf[ret] = (char *)buffer;
            buf_size[ret] = size;
            buf_off[ret] = 0;
        }
    }
    else        /* Some other invalid combination of parameters */
    {
        ret = -1;
        errno = EINVAL;
    }

    if (ret != -1)
    {
        fds[ret] = 0;
        buf_mode[ret] = *mode;
    }

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_lseek ()
-
-   Arguments   :   File descriptor, offset, whence
-
-   Returns     :   as per man lseek (2)
-
-   Description :   Does the same as lseek (2) except on a memory based file.
-                   Note: the memory area will be extended if the caller
-                   attempts to seek past the current end of file (memory).
-                   
--------------------------------------------------------------------------------
*/

int mfs_lseek (int fd, int offset, int whence)
{
    int ret;
    long test_off;

    if (fds[fd] == -1)  /* Not open */
    {
        ret = -1;
        errno = EBADF;
    }
    else if (offset < 0 && whence == SEEK_SET)
    {
        ret = -1;
        errno = EINVAL;
    }
    else
    {
        switch (whence)
        {
            case SEEK_SET:
                if (offset > buf_size[fd])
                    extend_mem_file (fd, offset);
                buf_off[fd] = offset;
                ret = offset;
                break;

            case SEEK_CUR:
                test_off = buf_off[fd] + offset;

                if (test_off < 0)
                {
                    ret = -1;
                    errno = EINVAL;
                }
                else
                {
                    if (test_off > buf_size[fd])
                        extend_mem_file (fd, test_off);
                    buf_off[fd] = test_off;
                    ret = test_off;
                }
                break;

            case SEEK_END:
                test_off = buf_size[fd] + offset;

                if (test_off < 0)
                {
                    ret = -1;
                    errno = EINVAL;
                }
                else
                {
                    if (test_off > buf_size[fd])
                        extend_mem_file (fd, test_off);
                    buf_off[fd] = test_off;
                    ret = test_off;
                }
                break;

            default:
                errno = EINVAL;
                ret = -1;
                break;
        }
    }

    return (ret);
}   

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_read ()
-
-   Arguments   :   File descriptor, buffer, size
-
-   Returns     :   as per man read (2)
-
-   Description :   Does the same as read (2) except on a memory based file.
-                   Note: An attempt to read past the end of memory currently
-                   allocated to the file will return 0 (End Of File)
-                   
--------------------------------------------------------------------------------
*/

int mfs_read (int fd, void *clnt_buf, int size)
{
    int ret;

    if (fds[fd] == -1 || buf_mode[fd] != 'r')
    {
        /* File is either not open, or not opened for read */

        ret = -1;
        errno = EBADF;
    }
    else if (buf_off[fd] + size > buf_size[fd])
    {
        ret = 0;        /* EOF */
    }
    else
    {
        memcpy (clnt_buf, (void *) (buf[fd] + buf_off[fd]), size);
        buf_off[fd] = buf_off[fd] + size;
        ret = size;
    }

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_write ()
-
-   Arguments   :   File descriptor, buffer, size
-
-   Returns     :   as per man write (2)
-
-   Description :   Does the same as write (2) except on a memory based file.
-                   Note: the memory area will be extended if the caller
-                   attempts to write past the current end of file (memory).
-                   
--------------------------------------------------------------------------------
*/

int mfs_write (int fd, void *clnt_buf, int size)
{
    int ret;

    if (fds[fd] == -1 || buf_mode[fd] == 'r')
    {
        /* Either the file is not open or it is opened for reading only */

        ret = -1;
        errno = EBADF;
    }
    else if (buf_mode[fd] == 'w')
    {
        /* Write */

        if (buf_off[fd] + size > buf_size[fd])
        {       
            extend_mem_file (fd, buf_off[fd] + size);
            buf_size[fd] = (buf_off[fd] + size);
        }

        memcpy ((buf[fd] + buf_off[fd]), clnt_buf, size);
        buf_off[fd] = buf_off[fd] + size;

        ret = size;
    }
    else
    {
        /* Append */

        if (buf_off[fd] != buf_size[fd])
            buf_off[fd] = buf_size[fd];

        extend_mem_file (fd, buf_off[fd] + size);
        buf_size[fd] += size;

        memcpy ((buf[fd] + buf_off[fd]), clnt_buf, size);
        buf_off[fd] = buf_off[fd] + size;

        ret = size;
    }

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_size ()
-
-   Arguments   :   File descriptor
-
-   Returns     :   integer file size
-
-   Description :   This function returns the current size of the file in bytes.
-                   
--------------------------------------------------------------------------------
*/

int mfs_size (int fd)
{
    int ret;

    if (fds[fd] == -1)  /* Not open */
    {
        ret = -1;
        errno = EBADF;
    }
    else
        ret = buf_size[fd];

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_map ()
-
-   Arguments   :   File descriptor, ptr to address, ptr to length
-
-   Returns     :   Map status (succeeded or otherwise)
-
-   Description :   This function tells the client where the file is mapped
-                   in memory and what size the mapped area is. It is provided
-                   to satisfy the MapProc function in libtiff. It pretends
-                   that the file has been mmap (2)ped.
-                   
--------------------------------------------------------------------------------
*/

int mfs_map (int fd, char **addr, size_t *len)
{
    int ret; 

    if (fds[fd] == -1)  /* Not open */
    {
        ret = -1;
        errno = EBADF;
    }
    else
    {
        *addr = buf[fd];
        *len = buf_size[fd];
        ret = 0;
    }

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_unmap ()
-
-   Arguments   :   File descriptor
-
-   Returns     :   UnMap status (succeeded or otherwise)
-
-   Description :   This function does nothing as the file is always
-                   in memory.
-                   
--------------------------------------------------------------------------------
*/

int mfs_unmap (int fd)
{
    return (0);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mfs_close ()
-
-   Arguments   :   File descriptor
-
-   Returns     :   close status (succeeded or otherwise)
-
-   Description :   Close the open memory file. (Make fd available again.)
-                   
--------------------------------------------------------------------------------
*/

int mfs_close (int fd)
{
    int ret; 

    if (fds[fd] == -1)  /* Not open */
    {
        ret = -1;
        errno = EBADF;
    }
    else
    {
        fds[fd] = -1;
        ret = 0;
    }

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   extend_mem_file ()
-
-   Arguments   :   File descriptor, length to extend to.
-
-   Returns     :   0 - All OK, -1 - realloc () failed.
-
-   Description :   Increase the amount of memory allocated to a file.
-                   
--------------------------------------------------------------------------------
*/

static int extend_mem_file (int fd, int size)
{
    void *new_mem;
    int ret;

    if ((new_mem = realloc (buf[fd], size)) == (void *) NULL)
        ret = -1;
    else
    {
        buf[fd] = (char *) new_mem;
        ret = 0;
    }

    return (ret);
}

/*
--------------------------------------------------------------------------------
-   Function    :   mem_init ()
-
-   Arguments   :   None
-
-   Returns     :   void
-
-   Description :   Initialise the library.
-                   
--------------------------------------------------------------------------------
*/

static void mem_init ()
{
    int i;

    for (i = 0; i < MAX_BUFFS; i++)
    {
        fds[i] = -1;
        buf[i] = (char *)NULL;
        buf_size[i] = 0;
        buf_off[i] = 0;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
