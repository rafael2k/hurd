/* Routines for dealing with '\0' separated arg vectors.

   Copyright (C) 1995 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.ai.mit.edu>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef __ARGZ_H__
#define __ARGZ_H__

/* Make a '\0' separated arg vector from a unix argv vector, returning it in
   ARGZ, and the total length in LEN.  If a memory allocation error occurs,
   ENOMEM is returned, otherwise 0.  The result can be destroyed using free. */
error_t argz_create(char **argv, char **argz, int *len);

/* Returns the number of strings in ARGZ.  */
int argz_count (char *argz, int len);

/* Puts pointers to each string in ARGZ into ARGV, which must be large enough
   to hold them all.  */
void argz_extract (char *argz, int len, char **argv);

/* Make '\0' separated arg vector ARGZ printable by converting all the '\0's
   except the last into the character SEP.  */
void argz_stringify(char *argz, int len, int sep);

/* Add BUF, of length BUF_LEN to the argz vector in ARGZ & ARGZ_LEN.  */
error_t
argz_append (char **argz, unsigned *argz_len, char *buf, unsigned buf_len);

/* Add STR to the argz vector in ARGZ & ARGZ_LEN.  This should be moved into
   argz.c in libshouldbelibc.  */
error_t argz_add (char **argz, unsigned *argz_len, char *str);

/* Delete ENTRY from ARGZ & ARGZ_LEN, if any.  */
void argz_delete (char **argz, unsigned *argz_len, char *entry);

/* Insert ENTRY into ARGZ & ARGZ_LEN before BEFORE, which should be an
   existing entry in ARGZ; if BEFORE is NULL, ENTRY is appended to the end.
   Since ARGZ's first entry is the same as ARGZ, argz_insert (ARGZ, ARGZ_LEN,
   ARGZ, ENTRY) will insert ENTRY at the beginning of ARGZ.  If BEFORE is not
   in ARGZ, EINVAL is returned, else if memory can't be allocated for the new
   ARGZ, ENOMEM is returned, else 0.  */
error_t
argz_insert (char **argz, unsigned *argz_len, char *before, char *entry);

/* Returns the next entry in ARGZ & ARGZ_LEN after ENTRY, or NULL if there
   are no more.  If entry is NULL, then the first entry is returned.  This
   behavior allows two convenient iteration styles: 

    char *entry = 0;
    while (entry = argz_next (argz, argz_len, entry))
      ...;

   or

    char *entry;
    for (entry = argz; entry; entry = argz_next (argz, argz_len, entry))
      ...;
*/
extern inline char *
argz_next (char *argz, unsigned argz_len, char *entry)
{
  if (entry)
    if (entry >= argz + argz_len)
      return 0;
    else
      return entry + strlen (entry) + 1;
  else
    if (argz_len > 0)
      return argz;
    else
      return 0;
}

#endif /* __ARGZ_H__ */