#include <num_sys.h>
#include <string.h>

char *dec_to_hex(uint64_t in) {
	/*Note: max hex value = 0xffffffffffffffff\0 */
	static char hex[19];
	hex[0] = '0';
	hex[1] = 'x';
	for(int i = 2; i < 18; i++) hex[i] = '0';

	hex[18] = '\0';

	int i = 17;

	for(; (i >= 2) && (in > 0); i--) {
		int overflow = in % 16;
		hex[i] = (overflow < 10) ? (overflow + '0') : (overflow + 'a' - 10);
		in = in / 16;
	}
	return hex;
}

char *dec_to_oct(uint64_t in) {
	/*Note: max oct value = 01777777777777777777777\0*/
	static char oct[24];
	oct[0] = '0';
	for(int i = 1; i < 23; i++) oct[i] = '0';
	oct[23] = '\0';

	int i = 22;

	for(; (i >= 1) && (in > 0); i--) {
		int overflow = in % 8;
		oct[i] = overflow + '0';
		in = in / 8;
	}

	return oct;
}



void lhex(char hex[19]) {
	for(int i = 2; i < 18; i++) {
		if(isalpha(hex[i])) hex[i] = tolower(hex[i]);
	}
}

void uhex(char hex[19]) {
	for(int i = 2; i < 18; i++) {
		if(isalpha(hex[i])) hex[i] = toupper(hex[i]);
	}
}