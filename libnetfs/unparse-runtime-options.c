/* A default netfs_unparse_runtime_options routine

   Copyright (C) 1995, 1996 Free Software Foundation, Inc.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include "netfs.h"

/* The user may define this function, in which case it is called when the the
   filesystem receives a get-options request.  ARGZ & ARGZ_LEN will contain
   information on `standard' netfs options; the user may extend them
   (probably by using argz_add), or ignore them, in which case case ARGZ
   should be freed, as it is malloced.  The default implementation simply
   leaves ARGZ & ARGZ_LEN unmodified and returns sucess (0).  */
error_t
netfs_unparse_runtime_options (char **argz, size_t *argz_len)
{
  return 0;
}