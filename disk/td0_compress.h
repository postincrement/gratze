#ifndef VIRTUAL_DRIVE_TDO_COMPRESS_H_
#define VIRTUAL_DRIVE_TDO_COMPRESS_H_

#include <stdint.h>
#include <string>
#include <vector>

struct TD0_Decoder
{
  TD0_Decoder(int fd, unsigned offs);

  int m_fd;
  unsigned m_offs;

  //////////////////////////////////////////////////

  static constexpr int BUFSZ     = 512;      // new input buffer

  /* LZSS Parameters */
  static constexpr int N         = 4096;     /* Size of string buffer */
  static constexpr int F         = 60;       /* Size of look-ahead buffer */
  static constexpr int THRESHOLD = 2;
  static constexpr int NIL       = N;        /* End of tree's node  */

  /* Huffman coding parameters */
  static constexpr int N_CHAR    = (256 - THRESHOLD + F); /* character code (= 0..N_CHAR-1) */
  static constexpr int T         = (N_CHAR * 2 - 1);      /* Size of table */
  static constexpr int R         = (T - 1);               /* root position */
  static constexpr int MAX_FREQ  = 0x8000;                /* update when cumulative frequency */
                                                   /* reaches to this value */
  void Init();
  void StartHuff();
  int Decode(uint8_t *buf, int len);
  int16_t DecodeChar();
  int16_t DecodePosition();
  int GetBit();    /* get one bit */
  int GetByte();    /* get a byte */
  void update(int c);
  int next_word();
  void reconst();
	int data_read(uint8_t *buf, uint16_t size);

  struct tdlzhuf {
    uint16_t r = 0,
            bufcnt = 0, bufndx = 0, bufpos = 0,  // string buffer
          // the following to allow block reads from input in next_word()
            ibufcnt = 0, ibufndx = 0; // input buffer counters
    uint8_t  inbuf[BUFSZ]{};    // input buffer
  } tdctl;

	uint8_t text_buf[N + F - 1];
	uint16_t freq[T + 1];    /* cumulative freq table */

  /*
  * pointing parent nodes.
  * area [T..(T + N_CHAR - 1)] are pointers for leaves
  */
	int16_t prnt[T + N_CHAR];

	/* pointing children nodes (son[], son[] + 1)*/
	int16_t son[T];

	uint16_t getbuf;
	uint8_t getlen;
};

#endif // VIRTUAL_DRIVE_TDO_COMPRESS_H_