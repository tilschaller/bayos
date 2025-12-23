#include <num_sys.h>

const char *dec_to_hex(uint64_t in) {
	/*Note: max hex value = 0xffffffffffffffff\0 */
	static char hex[19] = "0000000000000000";
	int i = 17;
	hex[0] = '0';
	hex[1] = 'x';
	hex[18] = '\0';

	if(in == 0) {
		hex[2] = '0';
		hex[3] = '\0';
		return hex;
	}

	for(; (i >= 2) && (in > 0); i--) {
		int overflow = in % 16;
		hex[i] = (overflow < 10) ? (overflow + '0') : (overflow + 'a' - 10);
		in = in / 16;
	}
	return hex;
}