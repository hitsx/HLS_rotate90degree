
#ifndef _FRAME_INTERLEAVER_H_
#define _FRAME_INTERLEAVER_H_

#include "ap_bmp.h"
#include "ap_video.h"

// I/O Image Settings
#define INPUT_IMAGE_BASE "test_1080p"
#define OUTPUT_IMAGE_BASE "result_1080p"

#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

#define LB_ROWS 24		// line buffer rows
#define WB_ROWS 12		// write back rows

#define PIXELDWIDTH 	24		// 24 bits for one pixel(RGB888)

#define MAX_CMD_NUM (MAX_WIDTH*MAX_HEIGHT/WB_ROWS)		//

struct AXI_PIXEL_IN{
  ap_uint<PIXELDWIDTH> data;
  ap_uint<1> user;
  ap_uint<1> last;
};

struct AXI_PIXEL_OUT{
  ap_uint<PIXELDWIDTH> data;
  ap_uint<1> user;
  ap_uint<1> last;
};

template<int D>
  struct cmd_axis{
	ap_uint<D> data;
  };

typedef cmd_axis<72> AXI_CMD;

typedef ap_linebuffer<ap_uint<PIXELDWIDTH>, WB_ROWS, MAX_WIDTH> RGB_BUFFER;

void frame_interleaver(	AXI_PIXEL_IN input_pix[MAX_HEIGHT][MAX_WIDTH],
		AXI_PIXEL_OUT output_pix[MAX_WIDTH][MAX_HEIGHT],
		AXI_CMD output_cmd[MAX_CMD_NUM],
		int mem_addr1, int mem_addr2, int mem_addr_index, int rows, int cols);

#endif
