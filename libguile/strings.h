/* classes: h_files */

#ifndef STRINGSH
#define STRINGSH
/*	Copyright (C) 1995,1996 Free Software Foundation, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * As a special exception, the Free Software Foundation gives permission
 * for additional uses of the text contained in its release of GUILE.
 *
 * The exception is that, if you link the GUILE library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the GUILE library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the
 * Free Software Foundation under the name GUILE.  If you copy
 * code from other Free Software Foundation releases into a copy of
 * GUILE, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for GUILE, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.  
 */


#include "libguile/__scm.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif



#define SCM_STRINGP(x) (SCM_TYP7S(x)==scm_tc7_string)
#define SCM_NSTRINGP(x) (!SCM_STRINGP(x))




extern SCM scm_string_p SCM_P ((SCM x));
extern SCM scm_read_only_string_p SCM_P ((SCM x));
extern SCM scm_string SCM_P ((SCM chrs));
extern SCM scm_makstr SCM_P ((long len, int slots));
extern SCM scm_makfromstrs SCM_P ((int argc, char **argv));
extern SCM scm_take0str SCM_P ((char * it));
extern SCM scm_makfromstr SCM_P ((const char *src, scm_sizet len, int slots));
extern SCM scm_makfrom0str SCM_P ((const char *src));
extern SCM scm_makfrom0str_opt SCM_P ((const char *src));
extern SCM scm_make_string SCM_P ((SCM k, SCM chr));
extern SCM scm_string_length SCM_P ((SCM str));
extern SCM scm_string_ref SCM_P ((SCM str, SCM k));
extern SCM scm_string_set_x SCM_P ((SCM str, SCM k, SCM chr));
extern SCM scm_substring SCM_P ((SCM str, SCM start, SCM end));
extern SCM scm_string_append SCM_P ((SCM args));
extern SCM scm_make_shared_substring SCM_P ((SCM str, SCM frm, SCM to));
extern void scm_init_strings SCM_P ((void));

#endif  /* STRINGSH */
