
#ifndef __BPE_COMPRESS_H_
#define __BPE_COMPRESS_H_

#define BPE_BLOCKSIZE 5000   // Maximum block size
#define BPE_HASHSIZE  4096   // Size of hash table
#define BPE_MAXCHARS   200   // Char set per block

class BpeCompress
{
public:
	// compress a buffer
	long compress (unsigned char* in, long insize, unsigned char* out)
	{
		int leftch, rightch, code, oldsize;
		int index, r, w, best, done = 0;

		input_buffer = in;
		input_buffer_size = insize;
		input_buffer_index = 0;
		output_buffer = out;
		output_buffer_index = 0;

		// write decompressed size
		_puts((unsigned char*)&insize, 4);

		// Compress each data block until end of file
		while (!done)
		{
			done = fetch();
			code = 256;

			// Compress this block
			for (;;) {

				// Get next unused char for pair code
				for (code--; code >= 0; code--)	{
					if (code==leftcode[code] && !rightcode[code])
						break;
				}

				// Must quit if no unused chars left
				if (code < 0) break;

				// Find most frequent pair of chars
				for (best=2, index=0; index<BPE_HASHSIZE; index++) {
					if (count[index] > best) {
						best = count[index];
						leftch = left[index];
						rightch = right[index];
					}
				}

				// Done if no more compression possible
				if (best < 3)
					break;

				// Replace pairs in data, adjust pair counts
				oldsize = size - 1;
				for (w = 0, r = 0; r < oldsize; r++) {
					if (buffer[r] == leftch && buffer[r+1] == rightch) {
						if (r > 0) {
							index = lookup(buffer[w-1],leftch);
							if (count[index] > 1) --count[index];
							index = lookup(buffer[w-1],code);
							if (count[index] < 255) ++count[index];
						}
						if (r < oldsize - 1) {
							index = lookup(rightch,buffer[r+2]);
							if (count[index] > 1) --count[index];
							index = lookup(code,buffer[r+2]);
							if (count[index] < 255) ++count[index];
						}
						buffer[w++] = code;
						r++; size--;
					} else
						buffer[w++] = buffer[r];
				}
				buffer[w] = buffer[r];

				// Add to pair substitution table
				leftcode[code] = leftch;
				rightcode[code] = rightch;

				// Delete pair from hash table
				index = lookup(leftch,rightch);
				count[index] = 1;
			}
			emit();
		}
		// Save size into header
		return output_buffer_index;
	}

protected:

	// Return index of character pair in hash table
	// Deleted nodes have count of 1 for hashing
	int lookup (unsigned char a, unsigned char b)
	{
		int index;

		// Compute hash key from both characters
		index = (a ^ (b << 5)) & (BPE_HASHSIZE-1);

		// Search for pair or first empty slot
		while ((left[index] != a || right[index] != b) && count[index] != 0)
			index = (index + 1) & (BPE_HASHSIZE-1);

		// Store pair in table
		left[index] = a;
		right[index] = b;
		return index;
	}

	// get the next char from input buffer
	int _getc()
	{
		if (input_buffer_index >= input_buffer_size) return -1;
		return input_buffer[input_buffer_index++];
	}

	// write a character to the output buffer
	void _putc(int c)
	{
		output_buffer[output_buffer_index++] = (unsigned char)c;
	}

	// write a string to the output buffer
	void _puts(unsigned char* buf, int len)
	{
		for (int i=0;i<len;i++)
			output_buffer[output_buffer_index++] = buf[i];
	}

	// Read next block from input buffer into buffer
	int fetch(void)
	{
		int c, index, used=0;

		// Reset hash table and pair table
		for (c = 0; c < BPE_HASHSIZE; c++) count[c] = 0;
		for (c = 0; c < 256; c++) {
			leftcode[c] = c;
			rightcode[c] = 0;
		}
		size = 0;

		// Read data until full or few unused chars
		while (size < BPE_BLOCKSIZE && used < BPE_MAXCHARS && (c = _getc()) != -1) {
			if (size > 0) {
				index = lookup(buffer[size-1],c);
				if (count[index] < 255) ++count[index];
			}
			buffer[size++] = c;

			// Use rightcode to flag data chars found
			if (!rightcode[c]) {
				rightcode[c] = 1;
				used++;
			}
		}
		return c == -1;
	}

	// Write each pair table and data block to output
	void emit()
	{
		int i, len, c = 0;

		// For each character 0..255
		while (c < 256)	{
			if (c == leftcode[c]) {
				// If not a pair code, count run of literals
				len = 1; c++;
				while (len<127 && c<256 && c==leftcode[c]) {
					len++;
					c++;
				}
				_putc(len + 127); len = 0;
				if (c == 256) break;
			} else {
				// Else count run of pair codes
				len = 0; c++;
				while (len<127 && c<256 && c!=leftcode[c] || len<125 && c<254 && c+1!=leftcode[c+1]) {
					len++;
					c++;
				}
				_putc(len);
				c -= len + 1;
			}

			// Write range of pairs to output
			for (i = 0; i <= len; i++) {
				_putc(leftcode[c]);
				if (c != leftcode[c])
					_putc(rightcode[c]);
				c++;
			}
		}

		// Write size bytes and compressed data block
		_putc(size/256);
		_putc(size%256);
		_puts(buffer,size);
	}

	unsigned char	buffer[BPE_BLOCKSIZE];	// Data block
	unsigned char	leftcode[256];		// Pair table
	unsigned char	rightcode[256];		// Pair table
	unsigned char	left[BPE_HASHSIZE];		// Hash table
	unsigned char	right[BPE_HASHSIZE];	// Hash table
	unsigned char	count[BPE_HASHSIZE];	// Pair count
	int size;							// Size of current data block

	unsigned char*	input_buffer;		// Input buffer
	long			input_buffer_size;	// Input buffer size
	long			input_buffer_index;	// current read offset

	unsigned char*	output_buffer;		// Output buffer
	long			output_buffer_index;	// Output Buffer size
};

#endif