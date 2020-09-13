/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <string.h>
#include <malloc.h>
#include <app.h>
#include <platform.h>
#include <kernel/thread.h>
#include <dev/keys.h>

//#define GPIO_TEST
#ifdef GPIO_TEST
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <platform/reg_physical.h>
#endif

static uint8_t *src;
static uint8_t *dst;

static uint8_t *src2;
static uint8_t *dst2;

#define BUFFER_SIZE (1024*1024)
#define ITERATIONS 16

extern void *mymemcpy(void *dst, const void *src, size_t len);
extern void *mymemset(void *dst, int c, size_t len);

static void *null_memcpy(void *dst, const void *src, size_t len)
{
	return dst;
}

static time_t bench_memcpy_routine(void *memcpy_routine(void *, const void *, size_t), size_t srcalign, size_t dstalign)
{
	int i;
	time_t t0;

	t0 = current_time();
	for (i=0; i < ITERATIONS; i++) {
		memcpy_routine(dst + dstalign, src + srcalign, BUFFER_SIZE);
	}
	return current_time() - t0;
}

static void bench_memcpy(void)
{
	time_t null, libc, mine;
	size_t srcalign, dstalign;
	
	printf("memcpy speed test\n");
	thread_sleep(200); // let the debug string clear the serial port

	for (srcalign = 0; srcalign < 64; ) {
		for (dstalign = 0; dstalign < 64; ) {

			null = bench_memcpy_routine(&null_memcpy, srcalign, dstalign);
			libc = bench_memcpy_routine(&memcpy, srcalign, dstalign);
			mine = bench_memcpy_routine(&mymemcpy, srcalign, dstalign);

			printf("srcalign %lu, dstalign %lu\n", srcalign, dstalign);
			printf("   null memcpy %u msecs\n", null);
			printf("   libc memcpy %u msecs, %llu bytes/sec\n", libc, BUFFER_SIZE * ITERATIONS * 1000ULL / libc);
			printf("   my   memcpy %u msecs, %llu bytes/sec\n", mine, BUFFER_SIZE * ITERATIONS * 1000ULL / mine);

			if (dstalign == 0)
				dstalign = 1;
			else 
				dstalign <<= 1;
		}
		if (srcalign == 0)
			srcalign = 1;
		else 
			srcalign <<= 1;
	}
}

static void fillbuf(void *ptr, size_t len, uint32_t seed)
{
	size_t i;

	for (i = 0; i < len; i++) {
		//for random
		//((char *)ptr)[i] = seed;
		//seed *= 0x1234567;

		// valky - for Repeatedly
		((char *)ptr)[i] = 0xF0;
	}
}

static void validate_memcpy(void)
{
	const size_t size = 1024*1024;
	/*!
	 * Start Address: 0x90000000
	 * End Address 	: 0xFFE00000
	 */
	src2 = memalign(64, size);	
	dst2 = memalign(64, size);	
	printf("src2:%x, dst2:%x\n", src2, dst2);

#ifdef GPIO_TEST
	//GPIO F16
	//base address : 0x74200000;
	*((unsigned int *)(0x74200178)) = 0x0; //GPFFNC2 GPIO FUNC -> 0(GPIO)
	*((unsigned int *)(0x74200144)) = 0x00010000; //GPFEN -> GPIO OUTPUT ENABLE
	gpio_config(TCC_GPF(16), GPIO_FN0|GPIO_OUTPUT|GPIO_LOW|GPIO_PULLDISABLE);
#endif
	while(1){
//		for (src = 0x83000000; src < 0x90000000; src += 1024*1024*2) {
		for (src = 0xFFE00000; src >= 0x90000000; src -= 1024*1024*2) {
			dst = src + 1024*1024;
			fillbuf(src, size, 567);
			fillbuf(src2, size, 567);
			fillbuf(dst, size, 123514);
			fillbuf(dst2, size, 123514);

#ifdef GPIO_TEST
			*((unsigned int *)(0x74200140)) = 0x00010000; //GPFDAT
			gpio_set(TCC_GPF(16), 1);
#endif
			memcpy(dst, src, size);
#ifdef GPIO_TEST
			*((unsigned int *)(0x74200140)) = 0x00000000; //GPFDAT
			gpio_set(TCC_GPF(16), 0);
#endif
			memcpy(dst2, src2, size);

			int comp = memcmp(dst2, dst, size);
			if (comp != 0) {
				printf("HALT: src:%x dst:%x size:%d memcpy FAILED!!!!!\n",src, dst, size);
				printf("HALT: src:%x dst:%x size:%d memcpy FAILED!!!!!\n",src, dst, size);
				printf("HALT: src:%x dst:%x size:%d memcpy FAILED!!!!!\n",src, dst, size);
				return;
			}else{
				printf("MEMCPY From:%x To:%x for Size:%d ----- OK\n", src, dst, size);
			}
		}
	}
}

static void validate_emmcr(void)
{
	unsigned long long ptn = 0;
	unsigned int page_size = flash_page_size();
	void *ptr;
	int ret;

	ptr = memalign(64, page_size * 2);
	fillbuf(ptr, page_size*2, 567);

	ptn = emmc_ptn_offset("splash");
	if(ptn == 0){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return;
	}

	ret = emmc_write(ptn, page_size, ptr);
	printf("EMMC INITIAL WRITE (size:%d) RESULT:%d\n",page_size, ret);

	while(1){
		ret = emmc_read(ptn, page_size, ptr);
		printf("EMMC READ (size:%d) TEST RESULT:%d\n",page_size, ret);
	}
}

static void validate_emmcw(void)
{
	unsigned long long ptn = 0;
	unsigned int page_size = flash_page_size();
	void *ptr;
	int ret;
	int cnt;

	ptr = memalign(64, page_size * 2);
	fillbuf(ptr, page_size*2, 567);

	ptn = emmc_ptn_offset("splash");
	if(ptn == 0){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return;
	}

	while(1){
		cnt = 200;
		while(cnt-->0){
			ret = emmc_write(ptn, page_size, ptr);
			printf("EMMC WRITE (size:%d) TEST RESULT:%d, COUNT:%d\n",page_size, ret, cnt);
		}
		cnt = 50;
		while(cnt-->0){
			printf("            !!COOLING TIME!! YOU CAN POWER OFF BEFORE COUNT BECOMES ZERO. COUNT:%d\n", cnt); 
		}
	}
}

static void validate_emmcrw(void)
{
	unsigned long long ptn = 0;
	unsigned int page_size = flash_page_size();
	void *ptr;
	void *ptr2;
	int ret;
	int cnt;

	ptr = memalign(64, page_size * 2);
	ptr2= memalign(64, page_size * 2);
	fillbuf(ptr, page_size*2, 567);

	ptn = emmc_ptn_offset("splash");
	if(ptn == 0){
		dprintf(CRITICAL, "ERROR : No splash partition found !\n");
		return;
	}

	while(1){
		cnt = 200;
		while(cnt-->0){
			fillbuf(ptr2, page_size*2, 123514);
			ret = emmc_write(ptn, page_size, ptr);
			printf("EMMC WRITE (size:%d) TEST RESULT:%d, COUNT:%d\n",page_size, ret, cnt);
			ret = emmc_read(ptn, page_size, ptr2);
			printf("EMMC READ (size:%d) TEST RESULT:%d\n",page_size, ret);
			ret = memcmp(ptr, ptr2, page_size);
			if(ret != 0){
				printf("             R/W DOES NOT MATCHED ERROR!!!!\n",ret);
			}else{
				printf("             R/W MATCHED - OK\n",ret);
			}
		}
		cnt = 50;
		while(cnt-->0){
			printf("            !!COOLING TIME!! YOU CAN POWER OFF BEFORE COUNT BECOMES ZERO. COUNT:%d\n", cnt); 
		}	
	}
}

static time_t bench_memset_routine(void *memset_routine(void *, int, size_t), size_t dstalign)
{
	int i;
	time_t t0;

	t0 = current_time();
	for (i=0; i < ITERATIONS; i++) {
		memset_routine(dst + dstalign, 0, BUFFER_SIZE);
	}
	return current_time() - t0;
}

static void bench_memset(void)
{
	time_t libc, mine;
	size_t dstalign;
	
	printf("memset speed test\n");
	thread_sleep(200); // let the debug string clear the serial port

	for (dstalign = 0; dstalign < 64; dstalign++) {

		libc = bench_memset_routine(&memset, dstalign);
		mine = bench_memset_routine(&mymemset, dstalign);

		printf("dstalign %lu\n", dstalign);
		printf("   libc memset %u msecs, %llu bytes/sec\n", libc, BUFFER_SIZE * ITERATIONS * 1000ULL / libc);
		printf("   my   memset %u msecs, %llu bytes/sec\n", mine, BUFFER_SIZE * ITERATIONS * 1000ULL / mine);
	}
}

static void validate_memset(void)
{
	size_t dstalign, size;
	int c;
	const size_t maxsize = 256;

	printf("testing memset for correctness\n");

	for (dstalign = 0; dstalign < 64; dstalign++) {
		printf("align %zd\n", dstalign);
		for (size = 0; size < maxsize; size++) {
			for (c = 0; c < 256; c++) {

				fillbuf(dst, maxsize * 2, 123514);
				fillbuf(dst2, maxsize * 2, 123514);

				memset(dst + dstalign, c, size);
				mymemset(dst2 + dstalign, c, size);

				int comp = memcmp(dst, dst2, maxsize * 2);
				if (comp != 0) {
					printf("error! align %zu, c %d, size %zu\n", dstalign, c, size);
				}
			}
		}
	}
}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

static int string_tests(int argc, cmd_args *argv)
{
	dprintf(INFO, "Memory Test Total Memory Size is 2GBytes\n");

	if (argc < 3) {
		printf("not enough arguments:\n");
usage:
		printf("%s validate <routine>\n", argv[0].str);
		printf("%s bench <routine>\n", argv[0].str);
		goto out;
	}

	if (!strcmp(argv[1].str, "validate")) {
		if (!strcmp(argv[2].str, "memcpy")) {
			validate_memcpy();
		} else if (!strcmp(argv[2].str, "memset")) {
			validate_memset();
		} else if (!strcmp(argv[2].str, "emmcr")) {
			validate_emmcr();
		} else if (!strcmp(argv[2].str, "emmcw")) {
			validate_emmcw();
		} else if (!strcmp(argv[2].str, "emmcrw")) {
			validate_emmcrw();
		}
	} else if (!strcmp(argv[1].str, "bench")) {
		if (!strcmp(argv[2].str, "memcpy")) {
			bench_memcpy();
		} else if (!strcmp(argv[2].str, "memset")) {
			bench_memset();
		}
	} else {
		goto usage;
	}

out:

	return 0;
}

void stringtests_init(const struct app_descriptor *app)
{
	cmd_args args[3] = {
		{ "", 0, 0 },
		{ "validate", 0, 0 },
//		{ "memcpy", 0, 0 },
//		{ "emmcr", 0, 0 },
		{ "emmcw", 0, 0 },
//		{ "emmcrw", 0, 0 },
	};
	
#if defined(PLATFORM_TCC) && defined(FWDN_V7) && defined(TCC_FWDN_USE)
    if (check_fwdn_mode() || keys_get_state(KEY_MENU) != 0) {
        fwdn_start();
        return;
    }

	string_tests(3, args);
#endif
}

STATIC_COMMAND_START
{ "string", NULL, &string_tests },
STATIC_COMMAND_END(stringtests);

#endif

APP_START(stringtests)
	.init = stringtests_init,
APP_END

