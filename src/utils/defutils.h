/*
 * The GPL License (GPLv3)
 *
 * Copyright © 2016 Thomas "Ventto" Venriès  <thomas.venries@gmail.com>
 *
 * Pearlfan is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pearlfan is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pearlfan.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef DEFUTILS_H_
#define DEFUTILS_H_

/* WARNING: Do not modify these values. */

#define PFAN_VID              0x0c45
#define PFAN_PID              0x7160

#define PFAN_MAX_W            144   // display width in pixels
#define PFAN_MAX_H            11    // display height in pixels
#define PFAN_MAX_DISPLAY      20    // number of display entries

#define MAX_PATH              1024

#define PFAN_OPEN             0
#define PFAN_CLOSE            1
#define PFAN_BEFORECLOSE      2

#define PFAN_IS_IMG           0
#define PFAN_IS_TXT           1

#endif /* !DEFUTILS_ */
