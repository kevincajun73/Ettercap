/*
    ettercap -- checksumming functions

    Copyright (C) ALoR & NaGA
    
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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <ec.h>
#include <ec_packet.h>
#include <ec_checksum.h>

/* protos... */

static u_int16 sum(u_int8 *buf, size_t len);
static u_int16 v4_checksum(struct packet_object *po);
static u_int16 v6_checksum(struct packet_object *po);

/*******************************************/


static u_int16 sum(u_int8 *buf, size_t len)
{
   /* create union for type punning - elements named as their rep. type */
   union {
      u_int8  u_int8 [4];
      u_int16 u_int16[2];
      u_int32 u_int32;
   } u;
   register u_int8 *cbuf = buf;
#if OS_SIZEOF_P == 8
   register u_int64 csum = 0;
#elif OS_SIZEOF_P == 4
   register u_int32 csum = 0;
#endif
   register unsigned int nleft = len;


#if OS_SIZEOF_P == 8
   while(nleft >= sizeof(u_int32)) {
      /* assigning bytes from buffer to union for 64-bit systems */
      u.u_int8[0] = cbuf[0];
      u.u_int8[1] = cbuf[1];
      u.u_int8[2] = cbuf[2];
      u.u_int8[3] = cbuf[3];

      csum += u.u_int32;
      cbuf += sizeof(u_int32);
      nleft -= sizeof(u_int32);
   }
#endif

   while(nleft >= sizeof(u_int16)) {
      /* assigning bytes from buffer to union for 32-bit systems */
      u.u_int8[0] = cbuf[0];
      u.u_int8[1] = cbuf[1];

      csum += u.u_int16[0];
      cbuf += sizeof(u_int16);
      nleft -= sizeof(u_int16);
   }

   if(nleft) {
      /* one word left */
      u.u_int8[0] = cbuf[0];
      u.u_int8[1] = 0;

      csum += u.u_int16[0];
      cbuf++;
      nleft--;
   }

#if OS_SIZEOF_P == 8
   csum = (csum >> 32) + (csum & 0xffffffff);
   csum = (csum >> 32) + (csum & 0xffffffff);
#endif

   csum = (csum >> 16) + (csum & 0xffff);
   csum += (csum >> 16);

   return csum;
}

/*
 * calculate the checksum of a Layer 3 packet
 */

u_int16 L3_checksum(u_char *buf, size_t len)
{
   u_int16 csum;
   csum = sum((u_int8 *) buf, len);

   return (u_int16)(~csum);
}

/*
 * calculate the checksum of a Layer 4 packet
 */

u_int16 L4_checksum(struct packet_object *po)
{
   switch(ntohs(po->L3.proto)) {
      case LL_TYPE_IP:  return v4_checksum(po);
      case LL_TYPE_IP6: return v6_checksum(po);
      default: break;
   }

   return 0;
}

static u_int16 v4_checksum(struct packet_object *po)
{
   u_int32 csum;
   int len = po->L4.len + po->DATA.len;

   csum = sum(po->L4.header, len);

   /* check the pseudo header */
   csum += po->L3.src.addr16[0];
   csum += po->L3.src.addr16[1];
   csum += po->L3.dst.addr16[0];
   csum += po->L3.dst.addr16[1];

   csum += htons((u_int16)po->L4.proto);
   csum += htons(len);

   csum = (csum >> 16) + (csum & 0xffff);
   csum += (csum >> 16);
   
   return (u_int16)(~csum);
}

static u_int16 v6_checksum(struct packet_object *po)
{
   u_int8 *buf = po->L4.header;
   u_int16 plen = po->L3.payload_len;
   u_int32 csum;

   csum = sum(buf, plen);
   
   csum += sum((uint8_t*)&po->L3.src.addr, ntohs(po->L3.src.addr_len));
   csum += sum((uint8_t*)&po->L3.dst.addr, ntohs(po->L3.dst.addr_len));
   csum += htons(plen + po->L4.proto);

   csum = (csum & 0xffff) + (csum >> 16);
   csum += (csum >> 16);

   return (u_int16)(~csum);
}

/*
 * calculate the CRC32 of a buffer
 */

u_int32 CRC_checksum(u_char *buf, size_t len, u_int32 init)
{
    static unsigned long crc_32_tab[] = {
       0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
       0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
       0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
       0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
       0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
       0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
       0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
       0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
       0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
       0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
       0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
       0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
       0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
       0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
       0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
       0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
       0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
       0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
       0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
       0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
       0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
       0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
       0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
       0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
       0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
       0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
       0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
       0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
       0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
       0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
       0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
       0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    
    u_int32 crc;
    size_t i;

    crc = init;
    
    for(i = 0; i < len; i++)
      crc = crc_32_tab[(crc ^ buf[i]) & 0xff] ^ (crc>>8);

    return crc;
}

/* stolen from wireshark source code (in_cksum.c) */

/*
 * Given the network-byte-order value of the checksum field in a packet
 * header, and the network-byte-order computed checksum of the data
 * that the checksum covers (including the checksum itself), compute
 * what the checksum field *should* have been.
 */
u_int16 checksum_shouldbe(u_int16 csum, u_int16 computed_sum)
{
	u_int32 shouldbe;

	/*
	 * The value that should have gone into the checksum field
	 * is the negative of the value gotten by summing up everything
	 * *but* the checksum field.
	 *
	 * We can compute that by subtracting the value of the checksum
	 * field from the sum of all the data in the packet, and then
	 * computing the negative of that value.
	 *
	 * "sum" is the value of the checksum field, and "computed_sum"
	 * is the negative of the sum of all the data in the packets,
	 * so that's -(-computed_sum - sum), or (sum + computed_sum).
	 *
	 * All the arithmetic in question is one's complement, so the
	 * addition must include an end-around carry; we do this by
	 * doing the arithmetic in 32 bits (with no sign-extension),
	 * and then adding the upper 16 bits of the sum, which contain
	 * the carry, to the lower 16 bits of the sum, and then do it
	 * again in case *that* sum produced a carry.
	 *
	 * As RFC 1071 notes, the checksum can be computed without
	 * byte-swapping the 16-bit words; summing 16-bit words
	 * on a big-endian machine gives a big-endian checksum, which
	 * can be directly stuffed into the big-endian checksum fields
	 * in protocol headers, and summing words on a little-endian
	 * machine gives a little-endian checksum, which must be
	 * byte-swapped before being stuffed into a big-endian checksum
	 * field.
	 *
	 * "computed_sum" is a network-byte-order value, so we must put
	 * it in host byte order before subtracting it from the
	 * host-byte-order value from the header; the adjusted checksum
	 * will be in host byte order, which is what we'll return.
	 */
	shouldbe = ntohs(csum);
	shouldbe += ntohs(computed_sum);
	shouldbe = (shouldbe & 0xFFFF) + (shouldbe >> 16);
	shouldbe = (shouldbe & 0xFFFF) + (shouldbe >> 16);
	return shouldbe;
}


/* EOF */

// vim:ts=3:expandtab

