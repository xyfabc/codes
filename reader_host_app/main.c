/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include "./include/libusb-1.0/libusb.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define VENDOR	0x0400
#define PRODUCT	0xc35a



#define BUF_LEN		8192

/*
 * struct test_state - describes test program state
 * @list: list of devices returned by libusb_get_device_list function
 * @found: pointer to struct describing tested device
 * @ctx: context, set to NULL
 * @handle: handle of tested device
 * @attached: indicates that device was attached to kernel, and has to be
 *            reattached at the end of test program
 */

struct test_state {
	libusb_device *found;
	libusb_context *ctx;
	libusb_device_handle *handle;
	int attached;
};

/*
 * test_init - initialize test program
 */

int test_init(struct test_state *state)
{
	int i, ret;
	ssize_t cnt;
	libusb_device **list;

	state->found = NULL;
	state->ctx = NULL;
	state->handle = NULL;
	state->attached = 0;

	ret = libusb_init(&state->ctx);
	if (ret) {
		printf("cannot init libusb: %s\n", libusb_error_name(ret));
		return 1;
	}

	cnt = libusb_get_device_list(state->ctx, &list);
	if (cnt <= 0) {
		printf("no devices found\n");
		goto error1;
	}

	for (i = 0; i < cnt; ++i) {
		libusb_device *dev = list[i];
		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(dev, &desc);
		if (ret) {
			printf("unable to get device descriptor: %s\n",
			       libusb_error_name(ret));
			goto error2;
		}
		if (desc.idVendor == VENDOR && desc.idProduct == PRODUCT) {
			state->found = dev;
			break;
		}
	}

	if (!state->found) {
		printf("no devices found\n");
		goto error2;
	}

	ret = libusb_open(state->found, &state->handle);
	if (ret) {
		printf("cannot open device: %s\n", libusb_error_name(ret));
		goto error2;
	}

	if (libusb_claim_interface(state->handle, 0)) {
		ret = libusb_detach_kernel_driver(state->handle, 0);
		if (ret) {
			printf("unable to detach kernel driver: %s\n",
			       libusb_error_name(ret));
			goto error3;
		}
		state->attached = 1;
		ret = libusb_claim_interface(state->handle, 0);
		if (ret) {
			printf("cannot claim interface: %s\n",
			       libusb_error_name(ret));
			goto error4;
		}
	}

	return 0;

error4:
	if (state->attached == 1)
		libusb_attach_kernel_driver(state->handle, 0);

error3:
	libusb_close(state->handle);

error2:
	libusb_free_device_list(list, 1);

error1:
	libusb_exit(state->ctx);
	return 1;
}

/*
 * test_exit - cleanup test program
 */

void test_exit(struct test_state *state)
{
	libusb_release_interface(state->handle, 0);
	if (state->attached == 1)
		libusb_attach_kernel_driver(state->handle, 0);
	libusb_close(state->handle);
	libusb_exit(state->ctx);
}
unsigned char cmd_link2dev[20]      = {0xaa, 0xaa, 0xaa, 0x96, 0x69 ,0x00, 0x03 ,0x12, 0xff, 0xee};
unsigned char cmd_FindId[20]        = {0xaa, 0xaa, 0xaa, 0x96, 0x69, 0x00, 0x03, 0x20, 0x01, 0x22};
unsigned char cmd_SelectID[20]      = {0xaa, 0xaa, 0xaa, 0x96, 0x69, 0x00, 0x03, 0x20, 0x02, 0x21};
unsigned char cmd_ReadBaseMsg[20]   = {0xaa, 0xaa, 0xaa, 0x96, 0x69, 0x00, 0x03, 0x30, 0x10, 0x23};


int main(void)
{
	struct test_state state;
	struct libusb_config_descriptor *conf;
	struct libusb_interface_descriptor const *iface;
	unsigned char in_addr, out_addr;
	static unsigned char buffer[BUF_LEN];
	int bytes;
	int i=0;
	if (test_init(&state))
		return 1;

	libusb_get_config_descriptor(state.found, 0, &conf);
	iface = &conf->interface[0].altsetting[0];
	in_addr = iface->endpoint[0].bEndpointAddress;
	out_addr = iface->endpoint[1].bEndpointAddress;

	// 连接设备并获取状态
  	 libusb_bulk_transfer(state.handle, out_addr,cmd_link2dev, 10, &bytes, 500);
   	 libusb_bulk_transfer(state.handle, in_addr, buffer, BUF_LEN, &bytes, 500);
    	while(1){
         //寻卡
        	libusb_bulk_transfer(state.handle, out_addr,cmd_FindId, 10, &bytes, 500);
        	libusb_bulk_transfer(state.handle, in_addr, buffer, BUF_LEN, &bytes, 500);
        	if(bytes>0){
			printf("find card (%d)\n",bytes);
		//	for(i=0;i<bytes;i++){
		//		printf(" 0x%x",buffer[i]);
		//		if(i%20==0){
		//		 	printf(" end\n");
		//		}

		//	}
		//	printf(" end\n");

			if(bytes==2321){
			printf("The card ID is \n");
			for(i=0;i<36;i+=2){
				printf(" %c",buffer[i+138]);

			}
			 printf("\n");
			}
			//寻卡成功并选卡
           		if(buffer[9] == 0x9f){
				printf("select id card (%d)\n",bytes);
                		libusb_bulk_transfer(state.handle, out_addr,cmd_SelectID, 10, &bytes, 500);
               			libusb_bulk_transfer(state.handle, in_addr, buffer, BUF_LEN, &bytes, 500);
				if(bytes>0){
	                          	printf("select id (%d)\n",bytes);
 
                		}else{
                         		printf("select id timetout\n");
                 		}

            		}
        	}else{
                	printf("cmd_findid timetout\n");
                }
		
		//读取卡号
       		libusb_bulk_transfer(state.handle, out_addr,cmd_ReadBaseMsg, 10, &bytes, 500);
        	libusb_bulk_transfer(state.handle, in_addr, buffer, BUF_LEN, &bytes, 1500);
        	if(bytes>0){
			 printf("rev msg (%d)\n",bytes);
			  //      for(i=0;i<bytes;i++){
                 //              printf(" 0x%x",buffer[i]);
                 //              if(i%20==0){

                 //                      printf(" end\n");                 //              }
 
                 //      }
                 //      printf(" end\n");
 
                         if(bytes==2321){
                         printf("The card ID is \n");
                         for(i=0;i<36;i+=2){
                                 printf(" %c",buffer[i+138]);
 
                         }
                          printf("\n");
                         }


        	}else{
			printf("timetout\n");
		}

   	 }

	test_exit(&state);
}
