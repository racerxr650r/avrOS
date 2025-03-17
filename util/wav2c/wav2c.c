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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>

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

#define DOUBLE_DASH_MAN_PAGE	"man-page"
#define FLAG_HELP				'?'
#define DESCR_HELP				"Display this help message"
#define FLAG_MAN_PAGE			0xff
#define DOUBLE_DASH_HELP		"help"
#define DESCR_MAN_PAGE			"Generate a man page and exit"
#define NOT_FLAG				0

#define local		static
#define persistant	static

typedef struct
{
	char dash;
	char *double_dash;
	char *parameters;
	char *description;
}flags_t;

typedef int (*flagParser_t)(char flag, char *arg[]);

void parseCommandLine(int argc, char *argv[], int flag_count, flags_t *flags, flagParser_t flagParser);

#define APPLICATION_DESCRIPTION	dash = 0
#define SUMMARY     	double_dash
#define PARAMETERS  	parameters
#define DESCRIPTION 	description

#define BOLD	"\fB"
#define ITALICS	"\fI"
#define RESET	"\fR"

// Application Data Types -----------------------------------------------------
typedef struct
{
	bool	compressed, replace, sign;
	int		deadband;
	int		frequency;
	int		buffer_size;
	char	*output_file, *input_file;
}app_config_t;

// Application Globals --------------------------------------------------------
app_config_t config =
{
	.compressed = false,
	.replace = false,
	.sign = false,
	.deadband = 5,
	.frequency = 8000,
	.buffer_size = 0xffff,
	.output_file = "snd.c",
	.input_file = NULL
};

flags_t flags[] =
{
	{
		.APPLICATION_DESCRIPTION,
		.SUMMARY =       "Utility to convert sound files to C source arrays",
		.PARAMETERS =    "[OPTIONS] <input sound file>",
		.DESCRIPTION =   "is a CLI app that converts various sound file formats into a C source code file "
						"that represents the sound data as a pulse code modulated (PCM) uint8_t or "
						"int8_t array depending on the selected options. The name of the array is the "
						"same as the source file without the extension. If the output C source file "
						"already exists, by default snd2c will append the array to end of the existing "
						"file. If compression is enabled, a deadband will be used to encode a run length "
						"of similar values. This deadband value represents a plus/minus value. So, a "
						"deadband of 5 is actually +/- 5. Larger value will increase the compression "
						"with a negative impact to the sound quality."
	},
	{
		.dash = 'b',
		.double_dash = "buffer",
		.parameters = "<ArraySize>",
		.description = "Integer value for the maximum size of the data buffer (default: 64K)"
	},
	{
		.dash = 'c',
		.double_dash = "compress",
		.parameters = NULL,
		.description = "Compress the output using RLE (default:no compression)"
	},
	{
		.dash = 'd',
		.double_dash = "deadband",
		.parameters = "<DeadbandValue>",
		.description = "+/- Integer value used for the deadband of the RLE compression (default: 5)"
	},
	{
		.dash = 'f',
		.double_dash = "frequency",
		.parameters = "<SampleFrequency>",
		.description = "Set the sample fequency of the ouput in Hz (default:8000 Hz)"
	},
	{
		.dash = 'o',
		.double_dash = "output",
		.parameters = "<Filename>",
		.description = "Name of the output C source code file (default:snd.c)"
	},
	{
		.dash = 'r',
		.double_dash = "replace",
		.parameters = NULL,
		.description = "Replace the output file if it already exists (default:append)"
	},
	{
		.dash = 's',
		.double_dash = "signed",
		.parameters = NULL,
		.description = "Signed integer output (default:unsigned)"
	},
};

int parseFlags(char flag, char *arg[])
{
	int ret;

	switch(flag)
	{
		case 'b':
			config.buffer_size = atoi(arg[0]);
			ret = 1;
			break;
		case 'c':
			config.compressed = true;
			ret = 0;
			break;
		case 'd':
			config.deadband = atoi(arg[0]);
			ret = 1;
			break;
		case 'f':
			config.frequency = atoi(arg[0]);
			ret = 1;
			break;
		case 'o':
			config.output_file = arg[0];
			ret = 1;
			break;
		case 'r':
			config.replace = true;
			ret = 0;
			break;	
		case 's':
			config.sign = true;
			ret = 0;
			break;
		case NOT_FLAG:
			// Set the input filename
			if(!config.input_file)
			{
				config.input_file = arg[0];
				ret = 1;
			}
			else
				ret = -1;
			break;
		default:
			// Parsing error
			ret = -1;
			break;
	}
	return(ret);
}

typedef enum
{
	NONE,
	INFO,
	WARN,
	ERROR,
	FATAL
}logLevel_t;

typedef enum
{
	QUIET,
	BASIC,
	COMPLETE
}logVerbosity_t;

local logLevel_t logLevel = FATAL;
local logVerbosity_t logVerbosity = QUIET;
FILE *logFile;

// This is a hack to get around a GCC standard c lib oddity
__attribute__((constructor)) void initHw(void) {
    logFile = stdout;
}

#define INFO(fmt_str,...)	do{logMessage(INFO,__FILE__,__FUNCTION__,__LINE__,fmt_str, ##__VA_ARGS__);}while(0)
#define WARN(fmt_str,...)	do{logMessage(WARN,__FILE__,__FUNCTION__,__LINE__,fmt_str, ##__VA_ARGS__);}while(0)
#define ERROR(fmt_str,...)	do{logMessage(ERROR,__FILE__,__FUNCTION__,__LINE__,fmt_str, ##__VA_ARGS__);}while(0)
#define FATAL(fmt_str,...)	do{logMessage(FATAL,__FILE__,__FUNCTION__,__LINE__,fmt_str, ##__VA_ARGS__);}while(0)

void setLogLevel(logLevel_t level)
{
	logLevel = level;
}

void setLogFile(FILE *file)
{
	logFile = file;
}

int setLogFileName(char *filename)
{
	FILE *file = fopen(filename,"w");
	
	if(file)
	{
		logFile = file;
		return(0);
	}
	else
		return(-1);
}

int logMessage(logLevel_t level, const char *file, const char *function, int line, const char *format, ...)
{
    va_list args;
    int result = 0;

	// Handle cases where the log file isn't open/valid
    if (logFile == NULL)
		return(-1);

	if(level>=logLevel)
	{
		// Preamble
		// If verbosity is BASIC or greater...
		if(logVerbosity==BASIC)
			result+=fprintf(logFile,"%s: ",
							level==FATAL?"FATAL":level==ERROR?"ERROR":level==WARN?"WARN":level==INFO?"INFO":"");
		// If verbosity is COMPLETE...
		if(logVerbosity==COMPLETE)
			fprintf(logFile,"%s:%s:%s():Line %d: ",
					level==FATAL?"FATAL":level==ERROR?"ERROR":level==WARN?"WARN":level==INFO?"INFO":"",
					file,function,line);

		va_start(args, format);
		// Use vfprintf for variable arguments
		result += vfprintf(logFile, format, args);  
		va_end(args);

		// New line
		fprintf(logFile,"\n");
		// Important: Flush the log file to ensure output is written immediately
		fflush(logFile);

		// If this is a FATAL event...
		if(logLevel==FATAL)
			exit(EXIT_FAILURE);
	}

    return result;
}

void displayUsage(FILE *output_stream, const char *command, int flag_count, flags_t flags[])
{
	fprintf(output_stream,
			"Usage: %s %s\n\r%s\n\n\rOPTIONS:\n\r",
			command,
			flags[0].PARAMETERS,
			flags[0].SUMMARY);
	
	for(int i=1;i<flag_count;++i)
		fprintf(output_stream,
				" -%c, --%-16s%-20s%s\n\r"
				,flags[i].dash,flags[i].double_dash,
				flags[i].parameters?flags[i].parameters:"",
				flags[i].description);

	fprintf(output_stream,
			"     --%-36s%s\n\r",
			DOUBLE_DASH_MAN_PAGE,
			DESCR_MAN_PAGE);
	fprintf(output_stream,
			" -%c, --%-16s                    %s\n\r",
			FLAG_HELP,
			DOUBLE_DASH_HELP,
			DESCR_HELP);

	fflush(output_stream);
}

/*
 * Display a message and exit the application with a given return code
 */
local void exitApp(char* error_str, int return_code)
{
   FILE *output;

   // Is the return code an error...
   if(return_code)
      output = stderr;
   else
      output = stdout;

   // If an error string was provided...
   if(error_str)
      if(strlen(error_str))
         fprintf(output, "%s %s\n\r%s %s\n\r",  
		 		return_code?"Error:":"OK:", 
                error_str, 
                return_code?"Error Code -":"", 
                return_code?strerror(errno):"");

   fflush(output);
   exit(return_code);
}

/*
 * Generate a man page
 */
void manPage(const char *command, int flag_count, flags_t flags[])
{
	// Finally, a legit use for a variable length array :-)
	char filename[strlen(command)+3];
	strcpy(filename,command);
	strcat(filename,".1");
	FILE *file = fopen(filename,"w");

	// If a file was actually opened...
	if(file!=NULL)
	{
		fprintf(file,".TH %s 1\n",command);
		fprintf(file,".SH NAME\n");
		fprintf(file,"%s \\- %s\n",command,flags[0].SUMMARY);
		fprintf(file,".SH SYNOPSIS\n");
		fprintf(file,".B %s %s\n",command,flags[0].PARAMETERS);
		fprintf(file,".SH DESCRIPTION\n");
		fprintf(file,".B %s\n",command);
		fprintf(file,"%s\n",flags[0].DESCRIPTION);
		fprintf(file,".SH OPTIONS\n");
		for(int i=1;i<flag_count;++i)
			fprintf(file,
					".TP\n.BR \\-%c \", \" \\-\\-%s \" \" \\fI%s\\fR\n%s\n",
					flags[i].dash,
					flags[i].double_dash,
					flags[i].parameters?flags[i].parameters:"",
					flags[i].description);
		fprintf(file,
				".TP\n.BR \\-\\-%s\n%s\n",
				DOUBLE_DASH_MAN_PAGE,
				DESCR_MAN_PAGE);
		fclose(file);

		exitApp("Generated man page",0);
	}
	else
		exitApp("Failed to generate man page",-1);
}

/*
 *	Get the dash flag given the double dash stting
 */
char getFlag(char *double_dash, int flag_count, flags_t flags[])
{
	// If this is the --man-page flag...
	if(!strcmp(double_dash,DOUBLE_DASH_MAN_PAGE))
		return(FLAG_MAN_PAGE);
	// If this is the --help flag...
	if(!strcmp(double_dash,DOUBLE_DASH_HELP))
		return(FLAG_HELP);
	// Else search the flags table for the flag..
	else
		for(int i=1;i<flag_count;++i)
			if(!strcmp(double_dash,flags[i].double_dash))
				return(flags[i].dash);
	
	// No double-dash flag found
	return(0);
}

/*
 * Extract the command name from a given full path string
 */
const char *extractCommandName(const char *fullPath)
{
    if (fullPath == NULL) 
        return NULL;

    char *lastSlash = strrchr(fullPath, '/');  // Find the last forward slash

    if (lastSlash != NULL)
        return lastSlash + 1; // Return pointer to character after the last slash
    else
        return fullPath;       // No slash found, so the whole string is the command name
}

/*
 * Parse the application command line and set up the configuration
 */
void parseCommandLine(int argc, char *argv[], int flag_count, flags_t *flags, flagParser_t flagParser)
{
	const char *commandName = extractCommandName(argv[0]);

	for(int i=1;i<=argc-1;++i)
	{
		char flag;
		int  increment_param;

		// If this is a double dash flag...
		if(argv[i][0]=='-' && argv[i][1]=='-')
		{
			flag = getFlag(&argv[i][2],flag_count,flags);
			// If no flag found...
			if(!flag)
			{
				displayUsage(stderr, commandName, flag_count, flags);
				FATAL("\nInvalid Command Line");
			}
		}
		// Else if this is a single dash flag...
		else if(argv[i][0]=='-')
			flag = argv[i][1];
		// Else this is not a flag...
		else
			flag = NOT_FLAG;

		switch(flag)
		{
			// --man-page flag
			case FLAG_MAN_PAGE:
				manPage(commandName, flag_count, flags);
				break;
			// Help
			case 'h':
			case '?':
				displayUsage(stdout, commandName, flag_count, flags);
				exitApp("",0);
				break;
			default:
				// If parser reports an error...
				increment_param = flagParser(flag,&argv[i+1]);
				if(increment_param == -1)
				{
					//fprintf(stderr,"Invalid Command Line\n\r");
					ERROR("Invalid Command Line");
					displayUsage(stderr, commandName, flag_count, flags);
					exit(-1);
				}
		}
		i+=increment_param;
	}
}

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
	int		source_samples;

	parseCommandLine(argc, argv, sizeof(flags)/sizeof(flags[0]), flags, parseFlags);

	if(argc == 6)
	{
		// If input file provided, open the file and load into buffer
		if(strcmp(INPUT_FILE,SINE_WAVE))
		{
			// Open FFMPEG to read the input file and output as 8 bit unsigned
			// raw data on a single (mono) channel at the specified sample rate.
			char cmd_line[250];
			FILE *pipeinput;
			sprintf(cmd_line,"ffmpeg -hide_banner -v 0 -i %s -f u8 -ac 1 -ar %s -",INPUT_FILE,SAMPLE_RATE);
			printf("Opening FFmeg pipe with command: %s\n\r",cmd_line);
			pipeinput = popen(cmd_line,"r");

			// Read the output from FFMPEG into the buffer and close the input
			// file. This will truncate the number of samples to 64K max.
			source_samples = fread(buff, 1, BUFF_SIZE, pipeinput);
			pclose(pipeinput);
			if(source_samples==0)
			{
				printf("Failure to pipe FFmpeg output.\n\r");
				return 1;
			}
			else
				printf("Read %d samples from the input file.\n\r",source_samples);
		}
		// Generate a 1 kHz sine wave and load into buffer
		else
		{
			int i;
			source_samples = atoi(SAMPLE_RATE)*atoi(DURATION);
			if(source_samples>sizeof(buff))
				source_samples = sizeof(buff);
			for (i=0 ; i<source_samples ; ++i)
				buff[i] = (uint8_t)(255.0 * ((sin(i*1000.0*2.0*M_PI/atof(SAMPLE_RATE))+1)/2));
			printf("Generated %d samples of a 1 kHz sine wave.\n\r",source_samples);
		}

		// Truncate the zeros at the end of the stream
		//for(;!buff[num];--num);

		// Copy the sampled data to another buffer compressing the data
		// using the deadband value provided in the commandline. Count
		// the resulting bytes as the new buffer is built
		uint8_t compressedBuff[BUFF_SIZE*7], lastValue;
		int i, byteCount = 0, rlCount = 0;
		for(i=0;i<(source_samples-1);++i)
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
		float duration = (float)source_samples/atof(SAMPLE_RATE);
		fprintf(fileoutput,"// Source file: %s\n",INPUT_FILE);
		fprintf(fileoutput,"// Sample Rate: %s Hz\n",SAMPLE_RATE);
		fprintf(fileoutput,"//     Samples: %d\n",source_samples);
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

		printf("Wrote %d additional bytes (%d samples, %f sec duration) the total file is %d bytes.\n\r",byteCount,source_samples,duration,totalByteCount);
	}
	else
		printf("Invalid command line.\n\rUsage: wav2c input_file sample_rate compression_deadband variable_name output_file\n\r");
}
