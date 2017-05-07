#include "ap_int.h"
#include "frame_interleaver.h"

//Main function for frame re-mapping
void frame_interleaver(	AXI_PIXEL_IN input_pix[MAX_HEIGHT][MAX_WIDTH],
		AXI_PIXEL_OUT output_pix[MAX_WIDTH][MAX_HEIGHT],
		AXI_CMD output_cmd[MAX_CMD_NUM],
		int mem_addr1, int mem_addr2, int& mem_addr_index, int rows, int cols)
{
  //Create AXI interfaces for the core
  #pragma HLS interface ap_fifo port=input_pix
  #pragma HLS resource core=AXI4Stream variable=input_pix metadata="-bus_bundle AXI4S_DATAS"
  #pragma HLS resource core=AXI4Stream variable=input_pix port_map={{input_pix_data_V TDATA} {input_pix_user_V TUSER} {input_pix_last_V TLAST}}

  #pragma HLS interface ap_fifo port=output_pix
  #pragma HLS resource core=AXI4Stream variable=output_pix metadata="-bus_bundle AXI4S_DATAM"
  #pragma HLS resource core=AXI4Stream variable=output_pix port_map={{output_pix_data_V TDATA} {output_pix_user_V TUSER} {output_pix_last_V TLAST}}

  #pragma HLS interface ap_fifo port=output_cmd
  #pragma HLS resource core=AXI4Stream variable=output_cmd metadata="-bus_bundle AXI4S_CMDM"
  #pragma HLS resource core=AXI4Stream variable=output_cmd port_map={{output_cmd_data_V TDATA}}

  #pragma HLS interface ap_none port=rows
  #pragma HLS resource core=AXI4LiteS metadata="-bus_bundle LITE" variable=rows
  #pragma HLS interface ap_none port=cols
  #pragma HLS resource core=AXI4LiteS metadata="-bus_bundle LITE" variable=cols
  #pragma HLS interface ap_none port=mem_addr1
  #pragma HLS resource core=AXI4LiteS metadata="-bus_bundle LITE" variable=mem_addr1
  #pragma HLS interface ap_none port=mem_addr2
  #pragma HLS resource core=AXI4LiteS metadata="-bus_bundle LITE" variable=mem_addr2
  #pragma HLS interface ap_vld port=mem_addr_index
  #pragma HLS resource core=AXI4LiteS metadata="-bus_bundle LITE" variable=mem_addr_index

  ap_uint<1> buf_use;

  int row_cnt;				// keeps the track of the destination lpddr memory address
  int cmd_index;

  int row, col;
  int row2, col2;
  int buff_row, buff_col;
  int buff_row_base;

  int offset;
  
  int frm_addr_offset_buf;
  static int frm_addr_sel = 1;
  #pragma HLS reset variable=frm_addr_sel off

  ap_uint<72> cmd;
  AXI_PIXEL_IN inputpix;
  AXI_PIXEL_OUT outputpix;

  RGB_BUFFER buff1;
  #pragma HLS resource core=RAM_2P_BRAM variable=buff1
  RGB_BUFFER buff2;
  #pragma HLS resource core=RAM_2P_BRAM variable=buff2

  // initial values
  cmd_index = 0;
  row_cnt = 0;
  row = 0;
  col = 0;
  row2 = 0;
  col2 = 0;
  buff_row = 0;
  buff_col = 0;
  buff_row_base = 0;
  offset = 0;

  // dual memory buffer selection at the begin of the loop
  if (frm_addr_sel == 2){
	  // inform the host processor mem_addr2 should not be changed
	  // due to the frm_addr_offset_buf is a register, the mem_addr_index
	  // should be implemented as inverse logic
	  mem_addr_index = 1;
	  // currently we use mem_addr2
	  frm_addr_offset_buf = mem_addr2;
		// make next selection to 1
	  frm_addr_sel = 1;
  }
  else{
	  // inform the host processor mem_addr1 should not be changed
	  // due to the frm_addr_offset_buf is a register, the mem_addr_index
	  // should be implemented as inverse logic
	  mem_addr_index = 2;
	  // currently we use mem_addr1
	  frm_addr_offset_buf = mem_addr1;
		// make next selection to 2
	  frm_addr_sel = 2;
  }

  LOOP1 : for(row = 0; row < rows + WB_ROWS; row++){
#pragma HLS loop_tripcount min=720+12 max=1080+12 avg=1080+12
#pragma HLS dependence variable=&buff1 false
#pragma HLS dependence variable=&buff2 false

	// decide buffer usage index
	if ((row / WB_ROWS) % 2 == 0) buf_use = 0;
	else buf_use = 1;

	LOOP2 : for(col = 0; col < cols; col++){
#pragma HLS loop_tripcount min=1280 max=1920 avg=1920
#pragma HLS loop_flatten off
#pragma HLS dependence variable=&buff1 false
#pragma HLS dependence variable=&buff2 false
#pragma HLS PIPELINE II = 1

		// for the last WB_ROWS rows of pixels, skip the input stream data acquire.
		if (row < rows){
			inputpix = input_pix[row][col];

			// Fill Line Buffer
			if (buf_use == 0){
				buff1.shift_up(col);
				buff1.insert_bottom(inputpix.data, col);
			}
			else {
				buff2.shift_up(col);
				buff2.insert_bottom(inputpix.data, col);
			}
		}

		// start write back after the first WB_ROWS rows of pixels fully received.
		if (row >= WB_ROWS){
			offset = frm_addr_offset_buf + (buff_row_base + rows - WB_ROWS - row_cnt) * (PIXELDWIDTH/8);

			if (buff_row == 0){
				// fill command stream
				cmd.range(22,0) = WB_ROWS*PIXELDWIDTH/8;	// WB_ROWS pixels of bytes to burst
				cmd.range(31,23) = 1;						// burst type set to 1, make the address change for every divided sub transfers
				cmd.range(63,32) = offset;
				cmd.range(71,64) = 0;
				output_cmd[cmd_index++].data = cmd;
			}
			
			if (buf_use == 0) outputpix.data = buff2.getval(buff_row, buff_col);
			else outputpix.data = buff1.getval(buff_row, buff_col);

			// make the rgbpack stage to re-sync for every WB_ROWS pixels
			if (buff_row == WB_ROWS - 1) outputpix.last = 1;
			else outputpix.last = 0;
			
			outputpix.user = 0;

			row2 = buff_col;
			col2 = rows - row_cnt + buff_row % WB_ROWS;
			output_pix[row2][col2] = outputpix;

			if (buff_col == cols - 1 && buff_row == WB_ROWS - 1){
				buff_row_base = 0;
				buff_col = 0;
				buff_row = 0;
				row_cnt += WB_ROWS;				// for every time of buffer write back, increase the row_cnt by WB_ROWS
			}
			else if (buff_row == WB_ROWS - 1){
				buff_row_base += rows;
				buff_row = 0;
				buff_col++;
			}
			else{
				buff_row++;
			}
		}
	}
  }
}
