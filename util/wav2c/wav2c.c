/*
 * wav2c.c - Read a wave file and export pcm data to a commma delimited
 *           file that can be used as source for C program.
 *
 * Requires: ffmpeg application (sudo apt-get install ffmpeg)
 * Compile:  gcc -Wall -o wav2c wav2c.c -lm
 *
 * Usage:    wav2c input_file sample_rate var_name output_file
 * 	input_file:		Filename of the input sound file.
 *	sample_rate:	Sample rate in Hz for the output.
 *  deadband:		+/- value for deadband of low energy pulses (compression)
 *	var_name:		Name of the uint8_t array containing the sound data.
 *	output_file:	Name of the header file to be created.
 *					Note: if the header file exists the new array will be
 *                        appended to the file.
 *
 * Created: 7/30/2019
 * Author : johna
 *
 * Copyright (C) 2019 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any 
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */ 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BUFF_SIZE   65535

#define INPUT_FILE	argv[1]
#define SAMPLE_RATE	argv[2]
#define COMPRESS_DB	argv[3]
#define VAR_NAME	argv[4]
#define OUTPUT_FILE	argv[5]

#define DURATION	argv[3]
#define COMMAND		argv[1]

#define SINE_WAVE	"sine_wave"

#define MAX_LINE_LEN	120

void outputValue(uint8_t value, uint8_t *buff, int end)
{
	static int line_len = 31;
	static int offset = 0;

	line_len += 6;
	// If max line length exceeded, start new line
	if(line_len > MAX_LINE_LEN)
	{
		buff[offset++] = '\n';
		line_len = 0;
	}
	if(!end)
		offset += sprintf((char *)&buff[offset],"0x%02x, ",value);
	else
		sprintf((char *)&buff[offset],"0x%02x};\n\n",value);
}

int main(int argc, char *argv[])
{
	uint8_t buff[BUFF_SIZE];
	int    num;

	if(argc == 6)
	{
		// If input file provided, open the file and load into buffer
		if(strcmp(INPUT_FILE,SINE_WAVE))
		{
			// Open FFMPEG to read the input file and output as 8 bit unsigned
			// raw data on a single (mono) channel at the specified sample rate.
			char cmd_line[250];
			FILE *pipeinput;
			sprintf(cmd_line,"ffmpeg -i %s -f s8 -ac 1 -ar %s -",INPUT_FILE,SAMPLE_RATE);
			printf("Opening ffmeg pipe with command: %s\n\r",cmd_line);
			pipeinput = popen(cmd_line,"r");

			// Read the output from FFMPEG into the buffer and close the input
			// file. This will truncate the number of samples to 64K max.
			num = fread(buff, 1, BUFF_SIZE, pipeinput);
			fclose(pipeinput);
			printf("Read %d samples from the input file.\n\r",num);
		}
		// Generate a 1 kHz sine wave and load into buffer
		else
		{
			int i;
			num = atoi(SAMPLE_RATE)*atoi(DURATION);
			if(num>sizeof(buff))
				num = sizeof(buff);
			for (i=0 ; i<num ; ++i)
				buff[i] = (uint8_t)(255.0 * ((sin(i*1000.0*2.0*M_PI/atof(SAMPLE_RATE))+1)/2));
			printf("Generated %d samples of a 1 kHz sine wave.\n\r",num);
		}

		// Truncate the zeros at the end of the stream
		//for(;!buff[num];--num);

		// Copy the sampled data to another buffer compressing the data
		// using the deadband value provided in the commandline. Count
		// the resulting bytes as the new buffer is built
		uint8_t compressedBuff[BUFF_SIZE*7], lastValue;
		int i, byteCount = 0, rlCount = 0;
		for(i=0;i<(num-1);++i)
		{
			if((buff[i]<=(0x80+atoi(COMPRESS_DB)))&&(buff[i]>=0x80-atoi(COMPRESS_DB)))
			{
				++rlCount;
				lastValue = buff[i];
				if(rlCount==0xff)
				{
					outputValue(0,compressedBuff,0);
					outputValue(rlCount,compressedBuff,0);
					byteCount += 2;
					rlCount = 0;
				}
			}
			else
			{
				if(rlCount==1)
				{
					outputValue(lastValue,compressedBuff,0);
					++byteCount;
					rlCount = 0;
				}
				else if(rlCount>1)
				{
					outputValue(0,compressedBuff,0);
					outputValue(rlCount,compressedBuff,0);
					byteCount += 2;
					rlCount = 0;
				}

				if(buff[i])
				{
					outputValue(buff[i],compressedBuff,0);
					++byteCount;
				}
				else
				{
					outputValue(1,compressedBuff,0);
					++byteCount;
				}
			}
		}
		if(rlCount>1)
		{
			outputValue(0,compressedBuff,0);
			outputValue(rlCount,compressedBuff,0);
		}
		else if(rlCount)
			outputValue(lastValue,compressedBuff,0);
		outputValue(buff[i],compressedBuff,1);

		// Open the output file and add the initial C text
		FILE *fileoutput;
		if(strcmp(INPUT_FILE,SINE_WAVE))
			fileoutput = fopen(OUTPUT_FILE,"a+");
		else
			fileoutput = fopen(SINE_WAVE,"a+");

		// Write the header w/stats for the stream
		float duration = (float)num/atof(SAMPLE_RATE);
		fprintf(fileoutput,"// Source file: %s\n",INPUT_FILE);
		fprintf(fileoutput,"// Sample Rate: %s Hz\n",SAMPLE_RATE);
		fprintf(fileoutput,"//     Samples: %d\n",num);
		fprintf(fileoutput,"//    Duration: %0.2f secs\n",duration);
		fprintf(fileoutput,"//    Deadband: +-%d\n",atoi(COMPRESS_DB));
		fprintf(fileoutput,"//        Size: %d Bytes\n", byteCount);
		fprintf(fileoutput,"const uint8_t %s[] PROGMEM = { ",VAR_NAME);

		// Write the stream data and close the output file
		fputs((char *)compressedBuff,fileoutput);

		rewind(fileoutput);

		int size = 0, totalByteCount = 0;
		while(fgets((char *)compressedBuff, 240, fileoutput))
			if(sscanf((char *)compressedBuff,"//        Size: %d Bytes",&size))
				totalByteCount += size;

		fclose(fileoutput);

		printf("Wrote %d additional bytes (%d samples, %f sec duration) the total file is %d bytes.\n\r",byteCount,num,duration,totalByteCount);
	}
	else
		printf("Invalid command line.\n\rUsage: wav2c input_file sample_rate compression_deadband variable_name output_file\n\r");
}
