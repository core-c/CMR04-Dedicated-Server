/*

Race Driver packets checksum 0.3
by Luigi Auriemma
e-mail: aluigi@autistici.org
web:    http://aluigi.altervista.org


INTRODUCTION
============

Finally this algorithm is opensource...
It is a modified MD5 algorithm used to calculate the 16bit checksum
placed at offset 1 of any UDP packet of the games Race Driver (both 1
and 2) developed by Codemasters and possibly also other games that use
the same checksum.

First of all a big thanx goes to REC, the Reverse Engineering Compiler
(http://www.backerstreet.com/rec/rec.htm) because I have used it to
create all the functions you see here and then I have modified them a
bit to work... this tool has saved a lot of my time.

In fact understanding the modifications between the original MD5
algorithm and that used in Race Driver is really hard so this has been
the only fast solution I have found.
In the past I used the dumped binary code of the algorithm but it
caused a lot of problems because parts of the memory were overwritten
after the calling of the function so you got a crash or some variables
were modified and then it ran only on Win32.

Now there are no problems!
The only limit seems to be the portability of the code because seems to
give a different result on my Ibook (PPC cpu).


USAGE
=====

    #include "rdcksum.h"

    u_char pck[] = "mypacket";

    rdcksum(pck, sizeof(pck) - 1);

The checksum is automatically applied to the packet (exactly where is
"yp") and is also returned by the function.
You don't need to set the checksum of the packet to zero because the
function does it automatically before calculating the checksum.


LICENSE
=======

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

#include <string.h>


#ifdef WIN32
    typedef unsigned char   u_char;
    typedef unsigned short  u_short;
    typedef unsigned long   u_long;
#endif



typedef struct md5_state_s {
    u_long  count[2];
    u_long  abcd[4];
    u_char  buf[64];
} md5_state_t;



void L00401BDC(
        u_long *A8,
        u_long Ac,
        u_long A10,
        u_long A14,
        u_long esp1c,
        u_long esp20,
        u_long A20) {
    u_long  eax;

    eax = ((~A14 | Ac) ^ A10) + esp1c;
    eax = *A8 + eax + A20;
    *A8 = ((eax >> (32 - esp20)) | (eax << esp20)) + Ac;
}



void L0040129C(md5_state_t *pms, u_long *esi) {
    u_long  ecx,
            edx,
            ebp,
            eax,
            ebx,
            esp1c,
            esp20,
            esp10,
            esp14;

    ecx = esi[0];
    edx = pms->abcd[1];
    ebp = pms->abcd[2];
    eax = pms->abcd[3];
    ebx = ~edx & eax;
    ecx = (ebx | (ebp & edx)) + pms->abcd[0] + ecx + 0xD76AA478;
    eax = ecx;
    ecx = ecx << 7;
    ebx = edx;
    eax = (eax >> 25 | ecx) + edx;
    ebx = ebx & eax;
    ecx = ((~eax & ebp) | ebx) + esi[1];
    ebx = pms->abcd[3] + ecx + -389564586;
    ecx = ebx;
    ebx = ebx << 12;
    ecx = (ecx >> 20 | ebx) + eax;
    ebx = ~ecx & edx;
    edx = (ebx | (ecx & eax)) + esi[2] + ebp + 606105819;
    ebx = edx;
    edx = edx << 17;
    ebx = (ebx >> 15 | edx) + ecx;
    edx = ~ebx & eax;
    edx = (edx | (ecx & ebx)) + esi[3];
    ebp = edx + pms->abcd[1] + -1044525330;
    edx = ebp;
    ebp = ebp >> 10;
    edx = (edx << 22 | ebp) + ebx;
    esp20 = ebx;
    ebp = ~edx;
    ebx = ebx & edx;
    eax = eax + ((ebp & ecx) | ebx) + esi[4] + -176418897;
    ebx = eax >> 25;
    ebx = (ebx | eax << 7) + edx;
    esp1c = ebx;
    eax = esi[5];
    ebx = ~ebx & esp20;
    ecx = ecx + (ebx | (edx & esp1c)) + eax + 1200080426;
    ebp = esp1c;
    eax = ecx;
    ecx = ecx << 12;
    eax = (eax >> 20 | ecx) + ebp;
    ecx = ~eax & edx;
    ecx = (ecx | (eax & ebp)) + esi[6];
    ebx = esp20 + ecx + -1473231341;
    ecx = ebx;
    ebx = ebx << 17;
    ecx = (ecx >> 15 | ebx) + eax;
    ebx = ~ecx & ebp;
    edx = edx + (ebx | (eax & ecx)) + esi[7] + -45705983;
    ebx = edx;
    edx = edx >> 10;
    ebx = (ebx << 22 | edx) + ecx;
    edx = ~ebx & eax;
    edx = (edx | (ecx & ebx)) + esi[8];
    ebp = edx + esp1c + 1770035416;
    edx = ebp;
    ebp = ebp << 7;
    edx = (edx >> 25 | ebp) + ebx;
    esp10 = ebx;
    ebx = ebx & edx;
    eax = eax + ((~edx & ecx) | ebx) + esi[9] + -1958414417;
    ebx = eax;
    eax = eax << 12;
    ebx = (ebx >> 20 | eax) + edx;
    eax = ~ebx & esp10;
    ecx = ecx + (eax | (ebx & edx)) + esi[10] + -42063;
    ebp = ecx;
    ecx = ecx << 17;
    ebp = (ebp >> 15 | ecx) + ebx;
    eax = ebx & ebp;
    ecx = ((~ebp & edx) | eax) + esi[11];
    ecx = esp10 + ecx + -1990404162;
    eax = ecx;
    ecx = ecx >> 10;
    eax = (eax << 22 | ecx) + ebp;
    esp20 = ebp;
    ebp = ebp & eax;
    ecx = ((~eax & ebx) | ebp) + esi[12];
    edx = edx + ecx + 1804603682;
    ecx = edx;
    edx = edx << 7;
    ecx = ecx >> 25 | edx;
    edx = esi[13];
    ecx = ecx + eax;
    esp1c = ecx;
    ecx = (~esp1c & esp20) | (eax & esp1c);
    ebp = esp1c;
    ebx = ebx + ecx + edx + -40341101;
    ecx = ebx;
    ebx = ebx << 12;
    ecx = (ecx >> 20 | ebx) + ebp;
    edx = ~ecx;
    esp14 = edx;
    edx = esp14 & eax;
    edx = (edx | (ecx & ebp)) + esi[14];
    ebx = esp20 + edx + -1502002290;
    edx = ebx;
    ebx = ebx << 17;
    edx = (edx >> 15 | ebx) + ecx;
    ebx = ~edx;
    esp20 = ebx;
    ebx = esp20 & ebp;
    eax = eax + (ebx | (ecx & edx)) + esi[15] + 1236535329;
    ebx = eax;
    eax = eax >> 10;
    ebp = ecx;
    ebx = ebx << 22 | eax;
    eax = esp14 & edx;
    ebx = ebx + edx;
    eax = (eax | (ebp & ebx)) + esi[1];
    ebp = eax + esp1c + -165796510;
    eax = ebp;
    ebp = ebp << 5;
    eax = eax >> 27 | ebp;
    esp20 = esp20 & ebx;
    eax = eax + ebx;
    ebp = edx & eax;
    esp1c = eax;
    ecx = ecx + (esp20 | ebp) + esi[6] + -1069501632;
    eax = ecx;
    ecx = ecx << 9;
    eax = eax >> 23 | ecx;
    ecx = esp1c;
    eax = eax + ecx;
    ebp = ~ebx & ecx;
    edx = edx + (ebp | (eax & ebx)) + esi[11] + 643717713;
    ecx = edx;
    edx = edx << 14;
    ecx = ecx >> 18 | edx;
    edx = esp1c;
    ebp = edx;
    ecx = ecx + eax;
    ebp = ~ebp & eax;
    esp20 = ecx;
    ebx = ebx + (ebp | (ecx & edx)) + esi[0] + -373897302;
    ecx = ebx << 20;
    ecx = ecx | ebx >> 12;
    ebx = esp20;
    edx = ~eax & ebx;
    ecx = ecx + ebx;
    edx = (edx | (eax & ecx)) + esi[5];
    ebp = edx + esp1c + -701558691;
    edx = ebp >> 27;
    esp1c = (edx | ebp << 5) + ecx;
    edx = ebx;
    ebp = ebx & esp1c;
    eax = eax + ((~edx & ecx) | ebp) + esi[10] + 38016083;
    edx = eax;
    eax = eax << 9;
    ebp = ~ecx;
    edx = edx >> 23 | eax;
    eax = esp1c;
    ebp = ebp & eax;
    edx = edx + eax;
    ebx = ebx + (ebp | (edx & ecx)) + esi[15] + -660478335;
    eax = ebx;
    ebx = ebx << 14;
    eax = eax >> 18 | ebx;
    eax = eax + edx;
    ebx = ~esp1c & edx;
    ecx = ecx + (ebx | (eax & esp1c)) + esi[4] + -405537848;
    ebx = ecx;
    ecx = ecx >> 12;
    ebx = (ebx << 20 | ecx) + eax;
    ecx = edx;
    ebp = edx & ebx;
    ecx = ((~ecx & eax) | ebp) + esi[9];
    ebp = ecx + esp1c + 568446438;
    ecx = ebp;
    ebp = ebp << 5;
    esp10 = ebx;
    ecx = (ecx >> 27 | ebp) + ebx;
    ebp = ~eax & ebx;
    edx = edx + (ebp | (eax & ecx)) + esi[14] + -1019803690;
    ebx = edx;
    edx = edx << 9;
    ebx = ebx >> 23 | edx;
    ebx = ebx + ecx;
    ebp = ebx & esp10;
    eax = eax + ((~esp10 & ecx) | ebp) + esi[3] + -187363961;
    edx = eax >> 18;
    edx = (edx | eax << 14) + ebx;
    eax = ecx;
    ebp = edx & ecx;
    eax = ((~eax & ebx) | ebp) + esi[8];
    ebp = eax + esp10 + 1163531501;
    eax = ebp;
    ebp = ebp >> 12;
    esp20 = edx;
    eax = eax << 20 | ebp;
    ebp = ebx;
    eax = eax + edx;
    ebp = ~ebp & edx;
    ecx = ecx + (ebp | (ebx & eax)) + esi[13] + -1444681467;
    edx = ecx;
    ecx = ecx << 5;
    esp1c = (edx >> 27 | ecx) + eax;
    edx = esp20;
    ecx = ~edx & eax;
    ebx = ebx + (ecx | (edx & esp1c)) + esi[2] + -51403784;
    ecx = ebx;
    ebx = ebx << 9;
    ecx = ecx >> 23 | ebx;
    ebx = esp1c;
    ecx = ecx + ebx;
    ebp = ~eax & ebx;
    ebx = edx + (ebp | (ecx & eax)) + esi[7] + 1735328473;
    edx = ebx;
    ebx = ebx << 14;
    edx = edx >> 18 | ebx;
    edx = edx + ecx;
    ebx = ~esp1c & ecx;
    eax = eax + (ebx | (edx & esp1c)) + esi[12] + -1926607734;
    ebx = eax << 20;
    ebx = (ebx | eax >> 12) + edx;
    ebp = esi[5];
    eax = (ecx ^ edx ^ ebx) + ebp;
    ebp = eax + esp1c + -378558;
    eax = ebp;
    ebp = ebp << 4;
    eax = (eax >> 28 | ebp) + ebx;
    ecx = ecx + (edx ^ ebx ^ eax) + esi[8] + -2022574463;
    ebp = ecx;
    ecx = ecx << 11;
    ebp = (ebp >> 21 | ecx) + eax;
    ecx = edx + (ebp ^ ebx ^ eax) + esi[11] + 1839030562;
    edx = ecx;
    ecx = ecx << 16;
    edx = (edx >> 16 | ecx) + ebp;
    ecx = ebp ^ edx;
    esp1c = ecx;
    ebx = ebx + (esp1c ^ eax) + esi[14] + -35309556;
    ecx = ebx;
    ebx = ebx >> 9;
    ecx = ecx << 23 | ebx;
    ecx = ecx + edx;
    ebx = eax + (esp1c ^ ecx) + esi[1] + -1530992060;
    eax = ebx;
    ebx = ebx << 4;
    eax = eax >> 28 | ebx;
    ebx = edx ^ ecx;
    eax = eax + ecx;
    ebp = (ebx ^ eax) + esi[4] + ebp + 1272893353;
    ebx = ebp;
    ebp = ebp << 11;
    ebx = (ebx >> 21 | ebp) + eax;
    edx = edx + (ebx ^ ecx ^ eax) + esi[7] + -155497632;
    ebp = edx;
    edx = edx << 16;
    ebp = (ebp >> 16 | edx) + ebx;
    edx = ebx ^ ebp;
    esp1c = edx;
    edx = ecx + (esp1c ^ eax) + esi[10] + -1094730640;
    ecx = edx << 23;
    ecx = (ecx | edx >> 9) + ebp;
    edx = eax + (esp1c ^ ecx) + esi[13] + 681279174;
    eax = edx;
    edx = edx << 4;
    eax = eax >> 28 | edx;
    edx = ebp ^ ecx;
    eax = eax + ecx;
    ebx = ebx + (edx ^ eax) + esi[0] + -358537222;
    edx = ebx;
    ebx = ebx << 11;
    edx = (edx >> 21 | ebx) + eax;
    ebp = (edx ^ ecx ^ eax) + esi[3] + ebp + -722521979;
    ebx = ebp;
    ebp = ebp << 16;
    ebx = (ebx >> 16 | ebp) + edx;
    ebp = edx ^ ebx;
    esp1c = ebp;
    ebp = ecx + (esp1c ^ eax) + esi[6] + 76029189;
    ecx = ebp;
    ebp = ebp >> 9;
    ecx = ecx << 23 | ebp;
    ecx = ecx + ebx;
    ebp = eax + (esp1c ^ ecx) + esi[9] + -640364487;
    eax = ebp;
    ebp = ebp << 4;
    eax = (eax >> 28 | ebp) + ecx;
    ebp = edx + (ebx ^ ecx ^ eax) + esi[12] + -421815835;
    edx = ebp;
    ebp = ebp << 11;
    edx = (edx >> 21 | ebp) + eax;
    ebp = ebx + (edx ^ ecx ^ eax) + esi[15] + 530742520;
    ebx = ebp;
    ebp = ebp << 16;
    ebx = (ebx >> 16 | ebp) + edx;
    ecx = ecx + (edx ^ ebx ^ eax) + esi[2] + -995338651;
    ebp = ecx << 23;
    ebp = (ebp | ecx >> 9) + ebx;
    ecx = eax + (( ~edx | ebp) ^ ebx) + esi[0] + -198630844;
    eax = ecx >> 26;
    eax = (eax | ecx << 6) + ebp;
    esp1c = eax;
    edx = edx + (( ~ebx | eax) ^ ebp) + esi[7] + 1126891415;
    ecx = edx;
    edx = edx << 10;
    ecx = (ecx >> 22 | edx) + eax;
    edx = (( ~ebp | ecx) ^ eax) + esi[14];
    eax = ~eax;
    ebx = ebx + edx + -1416354905;
    edx = ebx;
    ebx = ebx << 15;
    edx = edx >> 17 | ebx;
    ebx = esi[5];
    edx = edx + ecx;
    eax = ((eax | edx) ^ ecx) + ebx + ebp + -57434055;
    ebx = eax;
    eax = eax >> 11;
    ebx = ebx << 21 | eax;
    ebx = ebx + edx;
    esp14 = ecx;
    esp20 = edx;
    esp10 = ebx;
    L00401BDC(&esp1c, ebx, edx, ecx, esi[12], 6, 1700485571);
    L00401BDC(&esp14, esp1c, esp10, esp20, esi[3], 10, -1894986606);
    L00401BDC(&esp20, esp14, esp1c, esp10, esi[10], 15, -1051523);
    L00401BDC(&esp10, esp20, esp14, esp1c, esi[1], 21, -2054922799);
    L00401BDC(&esp1c, esp10, esp20, esp14, esi[8], 6, 1873313359);
    L00401BDC(&esp14, esp1c, esp10, esp20, esi[15], 10, -30611744);
    L00401BDC(&esp20, esp14, esp1c, esp10, esi[6], 15, -1560198380);
    L00401BDC(&esp10, esp20, esp14, esp1c, esi[13], 21, 1309151649);
    L00401BDC(&esp1c, esp10, esp20, esp14, esi[4], 6, -145523070);
    L00401BDC(&esp14, esp1c, esp10, esp20, esi[11], 10, -1120210379);
    L00401BDC(&esp20, esp14, esp1c, esp10, esi[2], 15, 718787259);
    L00401BDC(&esp10, esp20, esp14, esp1c, esi[9], 21, -343485551);
    pms->abcd[2] += esp20;
    pms->abcd[0] += esp1c;
    pms->abcd[1] += esp10;
    pms->abcd[3] += esp14;
}



void L00401C7C(md5_state_t *A4, u_char *A8, u_long Ac) {
    u_long  eax,
            esi,
            edi,
            ebp;

    eax = (A4->count[0] >> 3) & 63;
    A4->count[0] += (Ac << 3);
    if(A4->count[0] < (Ac << 3)) A4->count[1]++;
    A4->count[1] += (Ac >> 29);
    ebp = 64 - eax;
    if(Ac >= ebp) {
        memcpy(A4->buf + eax, A8, ebp);
        L0040129C(A4, (u_long *)A4->buf);
        esi = ebp + 63;
        edi = ebp;
        while(esi < Ac) {
            L0040129C(A4, (u_long *)(A8 + esi - 63));
            esi += 64;
            edi += 64;
        }
        eax = 0;
    } else {
        edi = 0;
    }
    memcpy(A4->buf + eax, A8 + edi, Ac - edi);
}



    /* Race Driver main function */

u_short rdcksum(u_char *data, u_long size) {
    md5_state_t     md5t;
    u_short         crc;
    u_long          tmp;
    static u_char   pad[] =
        "\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

    *(u_short *)(data + 1) = 0x0000;    // initialization

    md5t.count[0] = md5t.count[1] = 0;
    md5t.abcd[0] = 0x67452301;
    md5t.abcd[1] = 0xefcdab89;
    md5t.abcd[2] = 0x98badcfe;
    md5t.abcd[3] = 0x10325476;

    L00401C7C(&md5t, data, size);
    tmp = (md5t.count[0] >> 3) & 0x3f;
    if(tmp >= 56) tmp = 120 - tmp;
        else tmp = 56 - tmp;
    L00401C7C(&md5t, pad, tmp);
    L00401C7C(&md5t, (u_char *)&md5t.count[0], 8);

    crc = md5t.abcd[0] + md5t.abcd[1] + md5t.abcd[2] + md5t.abcd[3];

    *(u_short *)(data + 1) = crc;       // checksum

    return(crc);
}


