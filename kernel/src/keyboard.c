#include <keyboard.h>
#include <ports.h>
#include <types/types.h>

uint8_t shift = 0;

char scancode_to_char(uint8_t scancode) {
	switch(scancode) {
		// SHIFT keys
        	case 0x2A: shift = 1; return 0;   // Left Shift press
        	case 0x36: shift = 1; return 0;   // Right Shift press
        	case 0xAA: shift = 0; return 0;  // Left Shift release
        	case 0xB6: shift = 0; return 0;  // Right Shift release

        	// Number row
        	case 0x02: return shift ? '!' : '1';
        	case 0x03: return shift ? '@' : '2';
        	case 0x04: return shift ? '#' : '3';
        	case 0x05: return shift ? '$' : '4';
        	case 0x06: return shift ? '%' : '5';
        	case 0x07: return shift ? '^' : '6';
        	case 0x08: return shift ? '&' : '7';
        	case 0x09: return shift ? '*' : '8';
        	case 0x0A: return shift ? '(' : '9';
        	case 0x0B: return shift ? ')' : '0';
        	case 0x0C: return shift ? '_' : '-';
        	case 0x0D: return shift ? '+' : '=';
        	case 0x0E: return '\b';  // Backspace

        	// Top row letters
        	case 0x10: return shift ? 'Q' : 'q';
        	case 0x11: return shift ? 'W' : 'w';
        	case 0x12: return shift ? 'E' : 'e';
        	case 0x13: return shift ? 'R' : 'r';
        	case 0x14: return shift ? 'T' : 't';
        	case 0x15: return shift ? 'Z' : 'z'; // DE: Z instead of Y
        	case 0x16: return shift ? 'U' : 'u';
        	case 0x17: return shift ? 'I' : 'i';
        	case 0x18: return shift ? 'O' : 'o';
        	case 0x19: return shift ? 'P' : 'p';
        	case 0x1B: return shift ? '+' : '*'; // German: ß / ?

        	// Home row letters
        	case 0x1E: return shift ? 'A' : 'a';
        	case 0x1F: return shift ? 'S' : 's';
        	case 0x20: return shift ? 'D' : 'd';
        	case 0x21: return shift ? 'F' : 'f';
        	case 0x22: return shift ? 'G' : 'g';
        	case 0x23: return shift ? 'H' : 'h';
        	case 0x24: return shift ? 'J' : 'j';
        	case 0x25: return shift ? 'K' : 'k';
        	case 0x26: return shift ? 'L' : 'l';

        	// Bottom row letters
        	case 0x2C: return shift ? 'Y' : 'y'; // DE: Y instead of Z
        	case 0x2D: return shift ? 'X' : 'x';
        	case 0x2E: return shift ? 'C' : 'c';
        	case 0x2F: return shift ? 'V' : 'v';
        	case 0x30: return shift ? 'B' : 'b';
        	case 0x31: return shift ? 'N' : 'n';
        	case 0x32: return shift ? 'M' : 'm';
        	case 0x33: return shift ? ';' : ','; // German: , / ;
        	case 0x34: return shift ? ':' : '.'; // German: . / :
        	case 0x35: return shift ? '?' : '-'; // German: - / ?

        	// Special keys
        	case 0x39: return ' ';  // Space
        	case 0x1C: return '\n'; // Enter
        	case 0x0F: return '\t'; // Tab

        	default:
        	    return 0; // Unknown key
	}
}

char get_key(void) {
	uint8_t scancode = read_port_u8(0x60);

	return scancode_to_char(scancode);
}
