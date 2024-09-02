/*
 * keyb_tr.h - a keyboard layout definition
 *
 * Copyright (C) 2021 The EmuTOS development team
 *
 * Authors:
 *  LK   Levent Karakaş
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */


static const UBYTE keytbl_tr_norm[] = {
    0x00, 0x1b,  '1',  '2',  '3',  '4',  '5',  '6',
     '7',  '8',  '9',  '0',  '*',  '-', 0x08, 0x09,
     'q',  'w',  'e',  'r',  't',  'y',  'u', 0xfd,
     'o',  'p', 0xf0, 0xfc, 0x0d, 0x00,  'a',  's',
     'd',  'f',  'g',  'h',  'j',  'k',  'l', 0xfe,
     'i',  ',', 0x00, '\"',  'z',  'x',  'c',  'v',
     'b',  'n',  'm', 0xf6, 0xe7,  '.', 0x00, 0x00,
    0x00,  ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,  '-', 0x00, 0x00, 0x00, '+',  0x00,
    0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     '<', 0x00, 0x00,  '(',  ')',  '/',  '*',  '7',
     '8',  '9',  '4',  '5',  '6',  '1',  '2',  '3',
     '0',  '.', 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const UBYTE keytbl_tr_shft[] = {
    0x00, 0x1b,  '!', '\'',  '^',  '+',  '%',  '&',
     '/',  '(',  ')',  '=',  '?',  '_', 0x08, 0x09,
     'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
     'O',  'P', 0xd0, 0xdc, 0x0d, 0x00,  'A',  'S',
     'D',  'F',  'G',  'H',  'J',  'K',  'L', 0xde,
    0xdd,  ';', 0x00, 0xe9,  'Z',  'X',  'C',  'V',
     'B',  'N',  'M', 0xd6, 0xc7,  ':', 0x00, 0x00,
    0x00,  ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  '7',
     '8', 0x00,  '-',  '4', 0x00,  '6',  '+', 0x00,
     '2', 0x00,  '0', 0x7f, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     '>', 0x00, 0x00,  '(',  ')',  '/',  '*',  '7',
     '8',  '9',  '4',  '5',  '6',  '1',  '2',  '3',
     '0',  '.', 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const UBYTE keytbl_tr_caps[] = {
    0x00, 0x1b,  '1',  '2',  '3',  '4',  '5',  '6',
     '7',  '8',  '9',  '0',  '*',  '-', 0x08, 0x09,
     'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
     'O',  'P', 0xd0, 0xdc, 0x0d, 0x00,  'A',  'S',
     'D',  'F',  'G',  'H',  'J',  'K',  'L', 0xde,
    0xdd,  ',', 0x00, '\"',  'Z',  'X',  'C',  'V',
     'B',  'N',  'M', 0xd6, 0xc7,  '.', 0x00, 0x00,
    0x00,  ' ', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,  '-', 0x00, 0x00, 0x00,  '+', 0x00,
    0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     '<', 0x00, 0x00,  '(',  ')',  '/',  '*',  '7',
     '8',  '9',  '4',  '5',  '6',  '1',  '2',  '3',
     '0',  '.', 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


static const UBYTE keytbl_tr_altnorm[] = {
    0x10, '@',
    0x12, 0x80, /* € */
    0x13, 0xb6, /* ¶ */
    0x16, 0xfb, /* û */
    0x17, 0xee, /* î */
    0x18, 0xf4, /* ô */
    0x1a, 0xa8, /* ¨ */
    0x1b, '~',
    0x1e, 0xe2, /* â */
    0x1f, 0xdf, /* ß */
    0x21, 0xaa, /* ª */
    0x60, '|',
    0x2c, 0xab, /* « */
    0x2d, 0xbb, /* » */
    0x2e, 0xa2, /* ¢ */
    0x2f, 0x93, /* “ */
    0x30, 0x94, /* ” */
    0x31, 'n',
    0x32, 0xb5, /* µ */
    0x33, 0xd7, /* × */
    0x34, 0xb7, /* · */
    /* Keys from number line */
    0x27, '[',
    0x28, ']',
    0x29, '\\',
    0x35, '#',
    0,
};

static const UBYTE keytbl_tr_altshft[] = {
    0x13, 0xae, /* ® */
    0x15, 0xa5, /* ¥ */
    0x16, 0xdb, /* Û */
    0x17, 0xce, /* Î */
    0x18, 0xd4, /* Ô */
    0x1e, 0xc2, /* Â */
    0x60, 0xa6, /* ¦ */
    0x2b, 0xb0, /* ° */
    0x2c, '<',
    0x2d, '>',
    0x2e, 0xa9, /* © */
    0x2f, 0x91, /* ‘ */
    0x30, 0x92, /* ’ */
    0x31, 'N',
    0x32, 0xba, /* º */
    0x34, 0xf7, /* ÷ */
    /* Keys from number line */
    0x27, '{',
    0x28, '}',
    0x35, '$',
    0,
};

static const UBYTE keytbl_tr_altcaps[] = {
    0x10, '@',
    0x12, 0x80, /* € */
    0x13, 0xb6, /* ¶ */
    0x16, 0xdb, /* Û */
    0x17, 0xce, /* Î */
    0x18, 0xd4, /* Ô */
    0x1a, 0xa8, /* ¨ */
    0x1b, '~',
    0x1e, 0xc2, /* Â */
    0x1f, 0xdf, /* ß */
    0x21, 0xaa, /* ª */
    0x60, '|',
    0x2c, 0xab, /* « */
    0x2d, 0xbb, /* » */
    0x2e, 0xa2, /* ¢ */
    0x2f, 0x93, /* “ */
    0x30, 0x94, /* ” */
    0x31, 'N',
    0x32, 0xb5, /* µ */
    0x33, 0xd7, /* × */
    0x34, 0xb7, /* · */
    /* Keys from number line */
    0x27, '[',
    0x28, ']',
    0x29, '\\',
    0x35, '#',
    0,
};

static const struct keytbl keytbl_tr = {
    keytbl_tr_norm,
    keytbl_tr_shft,
    keytbl_tr_caps,
    keytbl_tr_altnorm,
    keytbl_tr_altshft,
    keytbl_tr_altcaps,
    NULL,
    0
};
