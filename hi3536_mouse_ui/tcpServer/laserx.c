/*
@ name: laserx.c
@ func: about laserx protocal process lib.

V0.0.1 @ 2014-06-05 17:15, by Techshino S.Y.C
	the first version
	
V0.0.2 @ 2014-06-12 21:18, by S.Y.C
	add a interface, laserx_recv.

V0.0.3 @ 2014-06-25 10:40, by S.Y.C
	fix the check_sum() bug, when len is 0.
	laserx_parse() add 0x30 check.

V0.0.4 @ 2014-06-27 10:52, by S.Y.C
	laserx_send() check len para when bin format.

V0.0.5 @ 2014-06-27 10:52, by S.Y.C
	laserx_callback_t struct change the send/recv type to int (*)(int, void *, int),
	compatible with the hardware layer interface.
*/


#include <string.h>
#include "laserx.h"




#define BUILD_INFO					"laserx_lib V0.0.5 "__DATE__" "__TIME__


/* frame header */
#define LX_FRAME_HEADER				'~'


#define SPLIT_BYTE(i,o)				({(o)[0] = 0x30 | ((i)>>4); \
									(o)[1] = 0x30 | ((i) & 0x0f);})


#define DEBUG_LASERX
#ifdef DEBUG_LASERX
    #include <stdio.h>
    #ifdef __GNUC__
        #define lx_debug(s,...)				printf(s, ##__VA_ARGS__)
    #else
        #define lx_debug					printf
    #endif
#else
    #ifdef __GNUC__
        #define lx_debug(s,...)				
    #else
        #define lx_debug(s)	
    #endif		
#endif



/* dynamic library */
#if defined(__GNUC__)
/* linux platform */
#ifdef BUILD_AS_DYNAMIC_LIB
#define NP_VISIBILITY_DEFAULT	__attribute__((visibility("default")))
#else
#define NP_VISIBILITY_DEFAULT
#endif

#define __DL(__type) 			NP_VISIBILITY_DEFAULT __type

#elif defined(WIN32)
/* windows platform */
//#if defined(_USRDLL)
//#define __DL(__type) 			_declspec(dllexport) __type
//#else
#define __DL(__type) 			__type
//#endif

#else
/* other platform */
#define __DL(__type) 			__type
#endif




static u8 check_sum(u8 *data, u32 len)
{
	u32 i;
	u8 ret;

	if (len == 0) {
		return 0;
	}

	ret = data[0];
	
	for (i=1; i<len; i++) {
		ret ^= data[i];
	}	
	
	return ret;
}



static u32 u8_u32_be(u8 *buf)
{
	u32 nu;
	
	nu = buf[0];
    nu <<= 8;
    nu |= buf[1];
    nu <<= 8;
    nu |= buf[2];
    nu <<= 8;
    nu |= buf[3];
	
	return nu;
}



static void u32_u8_be(u32 nu, u8 *buf)
{
	buf[0] = (u8)(nu >> 24);
	buf[1] = (u8)(nu >> 16);
	buf[2] = (u8)(nu >> 8);
	buf[3] = (u8)(nu >> 0);
}



static void split_byte(u8 i, u8 *o)
{
	o[0] = 0x30 | (i>>4);
	o[1] = 0x30 | (i & 0x0f);
}



/* split_decode, trans split data to bin data */
static void split_decode(u8 *in, u32 len, u8 *out)
{
	u32 i, j;
	u8 tmp;
	
	len >>= 1;
	
	for (i=0; i<len; i++) {
		j = i << 1;
		tmp = in[j];
		tmp <<= 4;
		tmp |= in[j+1] & 0x0f;
		
		out[i] = tmp;
	}	
}



/* split_encode, trans bin data to split data */
/*
static void split_encode(u8 *in, u8 *out, u32 len)
{
	u32 i, j;
	u8 data;
	
	for (i=0; i<len; i++) {
		j = i << 1;
		data = in[i];
		
		out[j] = 0x30 | (data>>4);
		out[j+1] = 0x30 | (data & 0x0f);
	}	
}*/



static void frame_set(laserx_frame_t *frame, u8 cmd, u8 format, 
					u8 *data, u32 len, u8 ret)
{		
	frame->cmd = cmd;
	frame->format = format;
	frame->data = data;
	frame->len = len;
	frame->ret = ret;
}


/* parse the command binary format frame */
static int parse_cmd_bin(u8 data, parse_para_t *para)
{
	u32 pos = para->pos;
	
	switch (pos) {
		case 0: {
			/* start receive frame header */
			if (data != LX_FRAME_HEADER) {
				lx_debug("head error\r\n");			
				return -1;
			}	
			break;
		}
		
		case 1: {
			/* receive frame format domain */
			if (data == LX_FORMAT_DET)	{
				/* parse done, ok */
				return 1;
			}			

			if ((data!=LX_FORMAT_BIN) && (data!=LX_FORMAT_SPT)) {
				lx_debug("format error\r\n"); 		
				return -1;
			}
			
			para->format = data;
			break;
		}
		
		case 2: {
			/* command code domain */
			/*para->cmd = data;*/
			break;
		}
		
		case 3:
		case 4:
		case 5:
		case 6: {
			/* length domain */
			para->frame_len <<= 8;
			para->frame_len |= data;
				
			if (pos == 6) {
				/* HDR FMT CMD LEN CHK, 8 byte */
				para->total_len = para->frame_len + 8; 
			}	
			
			break;
		}
		
		default : {
			/* reach the end */
			if (pos == (para->total_len-1)) {
				if ((para->sum^data^LX_FRAME_HEADER) != 0) {
					lx_debug("check sum error\r\n");
					return -1;
				}
				
				/* parse done, ok */				
				return 1;
			}
			break;
		}
	}
	
	/* update the counter */
	para->pos++;	
	para->sum ^= data;
		
	return 0;	
}



/* parse the acknowledge binary format frame */
static int parse_ack_bin(u8 data, parse_para_t *para)
{
	u32 pos = para->pos;
	
	switch (pos) {
		case 0: {
			/* start receive frame header */
			if (data != LX_FRAME_HEADER) {
				lx_debug("head error\r\n");
				return -1;
			}	
			break;
		}
			
		case 1: {
			/* receive frame format domain */
			if (data == LX_FORMAT_DET)	{
				/* parse done, ok */
				return 1;
			}			
			
			if ((data!=LX_FORMAT_BIN) && (data!=LX_FORMAT_SPT)) {
				lx_debug("format error\r\n"); 		
				return -1;
			}
			
			para->format = data;
			break;
		}
			
		case 2: {
			/* command code domain */
			/*
 			if (data != para->cmd) {
 				lx_debug("ack cmd not match\r\n");
				return -1;
			}*/
			break;
		}

		case 3: {
			/* return value domain */
			break;
		}
			
		case 4:
		case 5:
		case 6:
		case 7: {
			/* length domain */
			para->frame_len <<= 8;
			para->frame_len |= data;
			
			if (pos == 7) {
				/* HDR FMT CMD RET LEN(4) CHK, 9 byte */
				para->total_len = para->frame_len + 9; 
			}	
			
			break;
		}
			
		default : {
			/* reach the end */
			if (pos == (para->total_len-1)) {
				if ((para->sum^data^LX_FRAME_HEADER) != 0) {
					lx_debug("check sum error\r\n");
					return -1;
				}
				
				/* parse done, ok */				
				return 1;
			}
			break;
		}
	}
	
	/* update the counter */
	para->pos++;	
	para->sum ^= data;
	
	return 0;	
}



/* laserx_parse, parse the recieved data. when return <0, para must be clear.
 * @data: one byte data
 * @para: point to the parse para, it is needed in the parse process
 * @return: 0 not end, 1 parse ok, <0 fail */
__DL(int) laserx_parse(u8 data, parse_para_t *para, bool ack)
{	
	if (para->pos > 1) {
		/* trans split format to bin format and parse it */
		if (para->format == LX_FORMAT_SPT) {
			if ((data&0x30) != 0x30) {
				return -1;
			}

			if (para->nibble == 0) {
				para->half_byte = data & 0x0f;
				para->nibble = 1;
				return 0;
			}

			data &= 0x0f;
			data |=	para->half_byte << 4;
			para->nibble = 0;
		}
	}
	
	if (ack) {
		return parse_ack_bin(data, para);
	} else {
		return parse_cmd_bin(data, para);
	}
}



/* laserx_deframe, check all domain out. if the frame is split format,
trans it to bin format first, then check it out;
@data: point to the frame
@len: the frame totle length
@ack: TRUE make a ack frame, FALSE make a cmd frame
@frame: return the frame domain info */
__DL(void) laserx_deframe(u8 *data, u32 len, bool ack, laserx_frame_t *frame)
{	
	u32 format = data[1];
	
	if (format == LX_FORMAT_SPT) {
		split_decode(data+2, len-2, data+2);
	}

	memset(frame, 0, sizeof(*frame));
	
	if (ack) {
		frame_set(frame, data[2], data[1], data+8, u8_u32_be(data+4), data[3]);	
	} else {
		frame_set(frame, data[2], data[1], data+7, u8_u32_be(data+3), 0);
	}
}



/* laserx_enframe, build a laserx frame.
@buf: store the frame builded
@size: the size of the frame
@ack: TRUE make a ack frame, FALSE make a cmd frame
@return: 0 success, <0 fail */
__DL(int) laserx_enframe(laserx_frame_t *frame, u8 *buf, u32 *size, bool ack)
{
	u32 len = frame->len;
	u8 *data = frame->data;
	int offset = 2;
	u8 sum;
	
	buf[0] = LX_FRAME_HEADER;
	buf[1] = frame->format;
	
	switch (frame->format) {
		case LX_FORMAT_DET: {
			len = 2;
			break;
		}
		
		case LX_FORMAT_BIN: {
			buf[offset++] = frame->cmd;
			
			if (ack) {
				buf[offset++] = frame->ret;
			}
			
			/* fill the len domain, 4byte */
			u32_u8_be(len, buf+offset);
			offset += 4;
			memcpy(buf+offset, frame->data, len);
			offset += len;
			buf[offset] = check_sum(buf+1, offset-1);
			offset++;
			break;
		}
		
		case LX_FORMAT_SPT: {
			u8 tmp[4];
			u32 i;
			
			split_byte(frame->cmd, buf+offset);
			offset += 2;
			
			if (ack) {
				split_byte(frame->ret, buf+offset);
				offset += 2;
			}			
			
			u32_u8_be(len, tmp);
			
			for (i=0; i<4; i++) {
				split_byte(tmp[i], buf+offset);
				offset += 2;
			}
			
			for (i=0; i<len; i++) {
				split_byte(data[i], buf+offset);
				offset += 2;
			}

			sum = frame->format ^ frame->cmd 
				^ check_sum(data, len) ^ check_sum(tmp, 4);
			
			if (ack) {
				sum ^= frame->ret;
			}
			
			split_byte(sum, buf+offset);
			offset += 2;
			break;
		}
		
		default : {
			return -1;
		}
	}
	
	*size = offset;
	return 0;
}



/* laserx_send, send the frame according to the laserx frame arch
@frame: the frame info
@ack: TRUE as a ack frame, FALSE as a cmd frame
@send: the callback function, actual do the send */
__DL(int) laserx_send(laserx_frame_t *frame, bool ack, laserx_callback_t *cb)
{
	u32 len = frame->len;
	u8 *data = frame->data;
	int offset = 2;
	u8 buf[256];
	u8 sum;
	int err;
	
	buf[0] = LX_FRAME_HEADER;
	buf[1] = frame->format;
	
	switch (frame->format) {
		case LX_FORMAT_DET: {
			return cb->send(cb->dev, buf, 2);
		}
		
		case LX_FORMAT_BIN: {
			buf[offset++] = frame->cmd;
			
			if (ack) {
                printf("send ack \n");
				buf[offset++] = frame->ret;
            }else{
              printf("send not ack \n");
            }
			
			/* fill the len domain, 4byte */
			u32_u8_be(len, buf+offset);
			offset += 4;
			
			err = cb->send(cb->dev, buf, offset);
			
			if (err < 0) {
				return err;
			}
			
			if (len != 0) {
				err = cb->send(cb->dev, data, len);
				
				if (err < 0) {
					return err;
				}
			}
			
			sum = check_sum(buf+1, offset-1) ^ check_sum(data, len);
			
			return cb->send(cb->dev, &sum, 1);
		}
		
		case LX_FORMAT_SPT: {
			u32 buf_size;
			u8 tmp[4];
			u32 i;
			
			split_byte(frame->cmd, buf+offset);
			offset += 2;
			
			if (ack) {
				split_byte(frame->ret, buf+offset);
				offset += 2;
			}			
			
			u32_u8_be(len, tmp);
			
			for (i=0; i<4; i++) {
				split_byte(tmp[i], buf+offset);
				offset += 2;
			}
			
			err = cb->send(cb->dev, buf, offset);
			
			if (err < 0) {
				return err;
			}
			
			buf_size = sizeof(buf) >> 1;
			
			while (len > buf_size) {
				for (i=0; i<buf_size; i++) {
					split_byte(data[i], buf+(i<<1));
				}
				
				err = cb->send(cb->dev, buf, sizeof(buf));
			
				if (err < 0) {
					return err;
				}
				
				len -= buf_size;
				data += buf_size;
			}
			
			if (len != 0) {
				for (i=0; i<len; i++) {
					split_byte(data[i], buf+(i<<1));
				}
				
				err = cb->send(cb->dev, buf, len<<1);
			
				if (err < 0) {
					return err;
				}
			}
			
			sum = check_sum(frame->data, frame->len) ^ check_sum(tmp, 4);
			
			if (ack) {
				sum ^= frame->format ^ frame->cmd ^ frame->ret;
			} else {
				sum ^= frame->format ^ frame->cmd;
			}
			
			split_byte(sum, buf);
			
			return cb->send(cb->dev, buf, 2);
		}
		
		default : {
			return -1;
		}
	}
	
	return 0;
}


__DL(int) laserx_zmq_send(laserx_frame_t *frame, bool ack, laserx_callback_t *cb)
{
    u32 len = frame->len;
    u8 *data = frame->data;
    int offset = 2;
    u8 buf[256];
    u8 sum;
    int err;

    buf[0] = LX_FRAME_HEADER;
    buf[1] = frame->format;
 printf("-----3------NET_CMD_NVR_INFO\n");
    switch (frame->format) {
        case LX_FORMAT_DET: {
        //(*zmq_send) (void *s, const void *buf, size_t len, int flags);
            //return cb->send(cb->dev, buf, 2);
            return cb->zmq_send(cb->s, buf, 2,cb->zmq_flags);
        }

        case LX_FORMAT_BIN: {
            buf[offset++] = frame->cmd;

            if (ack) {
                printf("send ack \n");
                buf[offset++] = frame->ret;
            }else{
              printf("send not ack \n");
            }

            /* fill the len domain, 4byte */
            u32_u8_be(len, buf+offset);
            offset += 4;

            err = cb->zmq_send(cb->s, buf, offset,cb->zmq_flags);
                   // send(cb->dev, buf, offset);

            if (err < 0) {
                return err;
            }

            if (len != 0) {
                err =cb->zmq_send(cb->s, data, len,cb->zmq_flags);
                 //       cb->send(cb->dev, data, len);

                if (err < 0) {
                    return err;
                }
            }

            sum = check_sum(buf+1, offset-1) ^ check_sum(data, len);

            return cb->zmq_send(cb->s,  &sum, 1,cb->zmq_flags);

                    //cb->send(cb->dev, &sum, 1);
        }

        case LX_FORMAT_SPT: {
            u32 buf_size;
            u8 tmp[4];
            u32 i;

            split_byte(frame->cmd, buf+offset);
            offset += 2;

            if (ack) {
                split_byte(frame->ret, buf+offset);
                offset += 2;
            }

            u32_u8_be(len, tmp);

            for (i=0; i<4; i++) {
                split_byte(tmp[i], buf+offset);
                offset += 2;
            }

            err = cb->zmq_send(cb->s,  buf, offset,cb->zmq_flags);
                    //cb->send(cb->dev, buf, offset);

            if (err < 0) {
                return err;
            }

            buf_size = sizeof(buf) >> 1;

            while (len > buf_size) {
                for (i=0; i<buf_size; i++) {
                    split_byte(data[i], buf+(i<<1));
                }

                err =cb->zmq_send(cb->s,  buf, sizeof(buf),cb->zmq_flags);
                       // cb->send(cb->dev, buf, sizeof(buf));

                if (err < 0) {
                    return err;
                }

                len -= buf_size;
                data += buf_size;
            }

            if (len != 0) {
                for (i=0; i<len; i++) {
                    split_byte(data[i], buf+(i<<1));
                }

                err = cb->zmq_send(cb->s,  buf, len<<1,cb->zmq_flags);
                        //cb->send(cb->dev, buf, len<<1);

                if (err < 0) {
                    return err;
                }
            }

            sum = check_sum(frame->data, frame->len) ^ check_sum(tmp, 4);

            if (ack) {
                sum ^= frame->format ^ frame->cmd ^ frame->ret;
            } else {
                sum ^= frame->format ^ frame->cmd;
            }

            split_byte(sum, buf);

            return cb->zmq_send(cb->s,  buf, 2,cb->zmq_flags);
                   // cb->send(cb->dev, buf, 2);
        }

        default : {
            return -1;
        }
    }
 printf("----33-------NET_CMD_NVR_INFO\n");
    return 0;
}



/* laserx_recv, receive data and parse it. if have received a good frame, return
the frame architecture. if the frame is split format, it will be transed to bin format.
@buf: the buffer to receive data
@size: the buffer max size
@ack: TRUE as a ack frame, FALSE as a cmd frame
@cb: the callback function, actual do the receive
@frame: return the frame arch in the buffer */
__DL(int) laserx_recv(u8 *buf, u32 size, bool ack, 
					  laserx_callback_t *cb, laserx_frame_t *frame)
{
	u8 data;
	int err;
	u32 cnt = 0;
	parse_para_t para;

	memset(&para, 0, sizeof(para));

	while (1) {
		err = cb->recv(cb->dev, &data, 1);

		if (err < 0) {
			lx_debug("cb->recv() error, %d\r\n", err);
			return err;
		}


		err = laserx_parse(data, &para, ack);
		if (err < 0) {
			memset(&para, 0, sizeof(para));
			continue;
		}
		if (err > 0) {
			buf[cnt++] = data;
			laserx_deframe(buf, cnt, ack, frame);
			return 0;
		}
		if (err == 0) {
			if (cnt >= size) {
				/* buffer is not sufficient */
				lx_debug("buffer is not sufficient, bufsize: %d\r\n", size);
				return -1;
			}
			buf[cnt++] = data;
		}
	}
	return 0;
}



__DL(void) laserx_info(char buf[64])
{
	int len;
	int size = 64;
	
	memset(buf, 0, size);

	len = strlen(BUILD_INFO);

	len = (len>(size-1)) ? (size-1) : len;

	memcpy(buf, BUILD_INFO, len);
}



//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* FILE  END *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//
