/*

GSMSALG 0.2.1
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    http://aluigi.altervista.org


INTRODUCTION
------------
This algorithm is an emulation of that used by some game master servers
that support the "secure" Gamespy method (I don't know the official
name of this method, but secure is the name of a parameter sent by
these servers).
My implementation supports both the normal and the enctype 2 version
(that is just a simple operation more).


0.2 CHANGES
-----------
this new version of Gsmsalg has been created only for faster
performances but probably now the source code is sometimes a bit harder
to read or to convert in other languages.
There are NO other differences or critical updates, result is the same
The previous 0.1.1 version is now called gsmsalg-old.h and also it is
available on my website


USAGE EXAMPLE
-------------
The following is an usage example for RogerWilco with a secure string
equal to "ABCDEF":

    printf("%s\n", gsseckey("ABCDEF", "rW17Ko", 0));

And the following is an usage example for RogerWilco with a secure string
equal to "ABCDEF" but using enctype 2:

    printf("%s\n", gsseckey("ABCDEF", "rW17Ko", 2));

NOTE: the "normal" algorithm (0) is used for a lot of things, in fact
      not only to retrieve servers'list but also to send heartbeats
      packets and probably more.
NOTE: the enctype 2 receives encoded data, so you must know the
      algorithm to decode this data (IP and port) to read it.


LICENSE
-------
    Copyright 2004 Luigi Auriemma

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

    http://www.gnu.org/licenses/gpl.txt

*/



unsigned char gsvalfunc(unsigned char reg) {
    if(reg < 0x1a) return(reg + 'A');
    if(reg < 0x34) return(reg + 'G');
    if(reg < 0x3e) return(reg - 4);
    if(reg == 0x3e) return('+');
    if(reg == 0x3f) return('/');
    return(0);
}



unsigned char *gsseckey(unsigned char *secure, unsigned char *key, int enctype) {
    static  unsigned char   validate[9];
    unsigned char           secbuf[7],
                            buff[256],
                            *ptr,
                            *ptrval,
                            *sec,
                            *k,
                            tmp1,
                            tmp2,
                            ebx,
                            i,
                            ecx,
                            edx,
                            edi;

    i = 0;
    ptr = buff;
    do { *ptr++ = i++; } while(i);  /* 256 times */

    ptr = buff;
    k = memcpy(secbuf, key, 6); /* good if key is not NULLed */
    k[6] = edx = i = 0;
    do {    /* 256 times */
        if(!*k) k = secbuf;
        edx = *ptr + edx + *k;
            /* don't use the XOR exchange optimization!!! */
            /* ptrval is used only for faster code */
        ptrval  = buff + edx;
        tmp1    = *ptr;
        *ptr    = *ptrval;
        *ptrval = tmp1;
        ptr++; k++; i++;
    } while(i);

    sec = memcpy(secbuf, secure, 6);
    sec[6] = edi = ebx = 0;
    do {    /* 6 times */
        edi = edi + *sec + 1;
        ecx = ebx + buff[edi];
        ebx = ecx;
            /* don't use the XOR exchange optimization!!! */
            /* ptr and ptrval are used only for faster code */
        ptr     = buff + edi;
        ptrval  = buff + ebx;
        tmp1    = *ptr;
        *ptr    = *ptrval;
        *ptrval = tmp1;
        ecx = tmp1 + *ptr;
        *sec++ ^= buff[ecx];
    } while(*sec);

    if(enctype == 2) {
        ptr = key;
        sec = secbuf;
        do {    /* 6 times */
            *sec++ ^= *ptr++;
        } while(*sec);
    }

    sec = secbuf;
    ptrval = validate;
    for(i = 0; i < 2; i++) {
        tmp1 = *sec++;
        tmp2 = *sec++;
        *ptrval++ = gsvalfunc(tmp1 >> 2);
        *ptrval++ = gsvalfunc(((tmp1 & 3) << 4) + (tmp2 >> 4));
        tmp1 = *sec++;
        *ptrval++ = gsvalfunc(((tmp2 & 0xf) << 2) + (tmp1 >> 6));
        *ptrval++ = gsvalfunc(tmp1 & 0x3f);
    }
    *ptrval = 0x00;

    return(validate);
}


