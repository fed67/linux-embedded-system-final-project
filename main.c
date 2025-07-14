#define _POSIX_C_SOURCE_200112L

#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


struct gpiod_line_settings *settings;
struct gpiod_line_config *line_cfg;
struct gpiod_chip *chip;
static bool gpio_init = false;

enum gpio_direction { INPUT, OUTPUT};

#define GPIO_PIN 18


static int intit_device(const char *chip_path) {
	chip = gpiod_chip_open(chip_path);
	if (!chip)
		return -1;

	settings = gpiod_line_settings_new();
	if (!settings)
		goto close_chip;

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);

	return 0;

close_chip:
	gpiod_chip_close(chip);
	return -1;
}


static struct gpiod_line_request *
request_output_line_pull(const char *chip_path, unsigned int offset)
{
	struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_request *request = NULL;
	
	int ret;

	chip = gpiod_chip_open(chip_path);
	if (!chip)
		return NULL;

	settings = gpiod_line_settings_new();
	if (!settings)
		goto close_chip;

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_settings_set_drive(settings, GPIOD_LINE_BIAS_PULL_UP);

	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
    

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		goto free_settings;

	ret = gpiod_line_config_add_line_settings(line_cfg, &offset, 1,
						  settings);
	if (ret)
		goto free_line_config;

	const char* consumer = "Reset Onewire";
	if (consumer) {
		req_cfg = gpiod_request_config_new();
		if (!req_cfg)
			goto free_line_config;

		gpiod_request_config_set_consumer(req_cfg, consumer);
	}

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

	gpiod_request_config_free(req_cfg);

free_line_config:
	gpiod_line_config_free(line_cfg);

free_settings:
	gpiod_line_settings_free(settings);

close_chip:
	gpiod_chip_close(chip);

	return request;
}

static struct gpiod_line_request *
onewire_request_line(unsigned int offset, const char *consumer, enum gpio_direction dir)
{
	struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_request *request = NULL;
	
	int ret;

	if(gpio_init) {
		gpiod_line_config_free(line_cfg);
		gpiod_line_settings_free(settings);
	}

	settings = gpiod_line_settings_new();
	if (!settings)
		goto close_chip;

	if(OUTPUT) {
		gpiod_line_settings_set_direction(settings,
						GPIOD_LINE_DIRECTION_OUTPUT);

		gpiod_line_settings_set_drive(settings, GPIOD_LINE_DRIVE_OPEN_DRAIN );
	} else {
		gpiod_line_settings_set_direction(settings,
				GPIOD_LINE_DIRECTION_INPUT);
		gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
	}

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		goto free_settings;

	
	ret = gpiod_line_config_add_line_settings(line_cfg, &offset, 1,
						  settings);
	if (ret)
		goto free_line_config;

	if (consumer) {
		req_cfg = gpiod_request_config_new();
		if (!req_cfg)
			goto free_line_config;

		gpiod_request_config_set_consumer(req_cfg, consumer);
	}

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

	gpio_init = true;

	return request;

// 	gpiod_request_config_free(req_cfg);

free_line_config:
	gpiod_line_config_free(line_cfg);

free_settings:
	gpiod_line_settings_free(settings);

close_chip:
	gpiod_chip_close(chip);

	gpio_init = false;

	return request;
}


static int reset_pipe(const char *chip_path, unsigned int offset) {
	enum gpiod_line_value value[1];
	printf("reset_pipe \n");
	// struct gpiod_line_request* req = onewire_request_line(GPIO_PIN, "onewire", OUTPUT);

	struct gpiod_line_request* req = request_output_line_pull(chip_path, offset );
	printf("request_output_line_pull \n");

	size_t num_lines = gpiod_line_request_get_num_requested_lines(req);
	printf("pio num lines %u \n", num_lines);
	usleep(500);
	printf("END 1 US SLEEP 500 \n");

	value[0] = GPIOD_LINE_VALUE_INACTIVE;
	gpiod_line_request_set_values(req, value);

	printf("sleep 500us \n");
	usleep(500);

	printf("change direction \n");
	gpiod_line_settings_set_direction(settings,
		GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

	gpiod_line_config_reset(line_cfg);

	printf("add gpiod_line_config_add_line_settings \n");

	int ret = gpiod_line_config_add_line_settings(line_cfg, GPIO_PIN, 1,
						  settings);
	if (ret)
		goto free_line_config;

	printf("add gpiod_line_request_reconfigure_lines \n");
	ret = gpiod_line_request_reconfigure_lines(req, line_cfg );

	if (ret)
		goto free_line_config;


	printf("wait event");
	ret = gpiod_line_request_wait_edge_events(req, 1000);


	printf("result %d \n", ret);

free_line_config:
	gpiod_line_config_free(line_cfg);

free_settings:
	gpiod_line_settings_free(settings);

close_chip:
	gpiod_chip_close(chip);

	return -1;

}





static struct gpiod_line_request *
request_output_line(const char *chip_path, unsigned int offset,
		    enum gpiod_line_value value, const char *consumer, enum gpiod_line_drive drive,  bool set_drive)
{
	struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_request *request = NULL;
	
	int ret;

	chip = gpiod_chip_open(chip_path);
	if (!chip)
		return NULL;

	settings = gpiod_line_settings_new();
	if (!settings)
		goto close_chip;

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);

    if(set_drive)  {
        gpiod_line_settings_set_drive(settings, drive);
    } else {
    	gpiod_line_settings_set_output_value(settings, value);
    }

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		goto free_settings;

	ret = gpiod_line_config_add_line_settings(line_cfg, &offset, 1,
						  settings);
	if (ret)
		goto free_line_config;

	if (consumer) {
		req_cfg = gpiod_request_config_new();
		if (!req_cfg)
			goto free_line_config;

		gpiod_request_config_set_consumer(req_cfg, consumer);
	}

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

	gpiod_request_config_free(req_cfg);

free_line_config:
	gpiod_line_config_free(line_cfg);

free_settings:
	gpiod_line_settings_free(settings);

close_chip:
	gpiod_chip_close(chip);

	return request;
}



static enum gpiod_line_value toggle_line_value(enum gpiod_line_value value)
{
	return (value == GPIOD_LINE_VALUE_ACTIVE) ? GPIOD_LINE_VALUE_INACTIVE :
						    GPIOD_LINE_VALUE_ACTIVE;
}

static const char * value_str(enum gpiod_line_value value)
{
	if (value == GPIOD_LINE_VALUE_ACTIVE)
		return "Active";
	else if (value == GPIOD_LINE_VALUE_INACTIVE) {
		return "Inactive";
	} else {
		return "Unknown";
	}
}

/*
int main(int argc, char *argv[])
{
	// Example configuration - customize to suit your situation.
	static const char *const chip_path = "/dev/gpiochip0";
	static const unsigned int line_offset = 18;

	enum gpiod_line_value value = GPIOD_LINE_VALUE_ACTIVE;
    enum gpiod_line_drive drive = GPIOD_LINE_DRIVE_PUSH_PULL;
	struct gpiod_line_request *request;


    if(argc > 1) {
        switch(argv[1][0]) {
            case '0':
                value = GPIOD_LINE_VALUE_INACTIVE;
                break;
            case '1':
                value = GPIOD_LINE_VALUE_ACTIVE;
                break;
            case 'H':
                drive = GPIOD_LINE_BIAS_PULL_UP;
                break;
            case 'L':
                drive = GPIOD_LINE_BIAS_PULL_DOWN;
                break;
            case 'D':
                drive = GPIOD_LINE_DRIVE_PUSH_PULL;
                break;
			case 'r':
				reset_pipe(chip_path, line_offset);
				goto skip;
				break;
        }
    }

    printf("running cmd \n");
    if(argc > 1)
        printf("Command %c \n", argv[1][0]);


    request = request_output_line(chip_path, line_offset, value,
        "toggle-line-value", drive, false);



	if (!request) {
		fprintf(stderr, "failed to request line: %s\n",
			strerror(errno));
		return EXIT_FAILURE;
	}
	skip:

	// for (;;) {
	// 	printf("%d=%s\n", line_offset, value_str(value));
	// 	sleep(1);
	// 	value = toggle_line_value(value);
	// 	gpiod_line_request_set_value(request, line_offset, value);
	// }

	gpiod_line_request_release(request);

	return EXIT_SUCCESS;
}
*/

#define c_clk CLOCK_REALTIME

static const unsigned int line_offset_in = 14;
static const unsigned int line_offset_out = 18;

struct gpiod_request_config *req_cfg = NULL;
struct gpiod_line_request *request_loc = NULL;

struct gpiod_request_config *req_cfg_in = NULL;
struct gpiod_line_request *request_loc_in = NULL;

const char bit_mask[8] = { 
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128 };

unsigned int __attribute__((optimize("O0"))) my_wait(const int max) {
        unsigned int res = 1;
        unsigned int seed = time(0);
	for(int i = 0; i < max; i++) {
		 res = res * seed % 15;
	}
	return res;
}

uint8_t compute_crc(char* data, size_t length) {
	
	uint8_t result = 0x00;
	
	//char mask = 0x30;
	// 0x98 0x9800
	uint8_t mask = 0x31;
	
	for(int k = 0; k < length; k++) { //length
		uint8_t byte = data[k];
		//byte = 0x10;
	
	printf("byte %x\n", byte); 
	//result ^= byte;
	for( int i = 0; i < 8; i++) {
	        uint8_t mix = ( result ^= byte ) ^ 0x1;
		result >>= 1;
		if( mix ) {
			result = ( result ^ 0x8C );
			printf("skip bit i %i result %x \n", i, result);

		}
		byte >>= 1; 
			
	}
	printf("CRC %x result %x data %x \n", result, result, data[k]);
	}
	
	return result;
}	
	

int write_cmd(struct gpiod_line_request *request_out, char* data, size_t length) {
	int ret = -1;

	
	for(int i = 0; i < length; i++) {
		
		for(int j = 0; j < 8; j++) {
			if( data[i] & bit_mask[j] ) {
				printf("Write 1 \n");
				ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_INACTIVE);
				//usleep(1);
				my_wait(20);
				
				
				ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);
				usleep(60);
			} else {
				printf("Write 0 \n");
				ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_INACTIVE);
				usleep(60);
				ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);
				usleep(15);
			}
		}
		usleep(30);
	}		

	return 0;
}


int read_cmd(struct gpiod_line_request *request_out, struct gpiod_line_request *request_in, char* data, size_t length) {
	int ret,ret2 = -1;

	const int event_buf_size = 100;
	struct gpiod_edge_event_buffer* buff = gpiod_edge_event_buffer_new(event_buf_size);

	ret = 1;
	//do {
	//	ret = gpiod_line_request_wait_edge_events(request_loc_in, 1000);
	//	printf("event \n");
	//} while(ret == 1);
	
	ret = gpiod_line_request_read_edge_events(request_in,
				buff, 100);
	if(ret == event_buf_size) {
		printf("Error edge detection event buffer is too small \n");
		return -1;
	}
	struct timespec start_t;
	struct timespec now_t;
	for(int i = 0; i < length; i++) {
		char read_bits = 0;
		for(int j = 0; j < 8; j++) {
			ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_INACTIVE);
			//my_wait(209);
			
			
			ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);
			//ret = gpiod_line_request_wait_edge_events(request_in, 1000);
			ret - gpiod_line_request_wait_edge_events(request_in,
				16000);
			printf("ret wait %i \n", ret);
			//usleep(50);
			ret = gpiod_line_request_read_edge_events(request_in,
				buff, event_buf_size);
			//for(int k = 0; k < ret; k++) {
			//	struct gpiod_edge_event* event;
			//	event = gpiod_edge_event_buffer_get_event(buff,k);
			//	printf("k %i event %i %i \n", k, 
			//	gpiod_edge_event_get_event_type( event));
			//}
							
			//enum gpiod_line_value read = gpiod_line_request_get_value(request_in, line_offset_in);
	//printf("got value %i \n", read);
			printf("ret %i \n", ret);
			if( ret == 2 ) {
				printf("Read 0 \n");
				read_bits = read_bits >> 1;
			} else if(ret == 1) {
				printf("Read 1 %i \n", ret);	
				read_bits = (read_bits >> 1) | 0x80; //put a '1' at bit 7		
				
			} else {
				printf("Read error too many events read %i \n", ret);
			}
			usleep(60);
		}
		data[i] = read_bits;
		printf("------ readd ata %x  -------------\n", read_bits);
	}		

	return 0;
}

int reset(struct gpiod_line_request *request_out, struct gpiod_line_request *request_in) {

	int ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_INACTIVE);

	struct timespec dur;
	dur.tv_sec = 0;
	dur.tv_nsec = 500000;
	struct timespec remain;

	nanosleep(&dur, &remain);

	ret = gpiod_line_request_set_value(request_out, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);

	//rising edge from host
	ret = gpiod_line_request_wait_edge_events(request_in, -1);
	
	//rising edge rise client
	ret = gpiod_line_request_wait_edge_events(request_in, -1);

}

int CC(struct gpiod_line_request *request_out) {
	char data[1] = { 0xCC };
	write_cmd(request_out, data, 1);
	usleep(100);
}

int main(int argc, char *argv[])
{
	/* Example configuration - customize to suit your situation. */
	static const char *const chip_path = "/dev/gpiochip0";


	// enum gpiod_line_value value = GPIOD_LINE_VALUE_ACTIVE;
    // enum gpiod_line_drive drive = GPIOD_LINE_DRIVE_PUSH_PULL;
	//Wstruct gpiod_line_request *request;
	
	struct timespec start_t;
	struct timespec now_t;



	
	int ret;
	
	clock_t start;


	struct gpiod_line_settings *settings_ouptut;
	struct gpiod_line_config *line_cfg_out;
	struct gpiod_line_settings *settings_input;
	struct gpiod_line_config *line_cfg_in;
	
	struct gpiod_line_settings *settings_input_low;
	struct gpiod_line_config *line_cfg_in_low;
	
	int event_buf_size = 5;
	struct gpiod_edge_event_buffer *event_buffer;

	chip = gpiod_chip_open(chip_path);
	if (!chip)
		return -1;

	settings = gpiod_line_settings_new();
	if (!settings)
		goto close_chip;

        event_buffer = gpiod_edge_event_buffer_new(event_buf_size);
        if (!event_buffer) {
        	printf("Error when creating event buffer %s \n", strerror(errno));
        	goto close_chip;
        }
	
	settings_ouptut = gpiod_line_settings_new();
	ret = gpiod_line_settings_set_direction(settings_ouptut, GPIOD_LINE_DIRECTION_OUTPUT);
	printf("direction %i \n", ret);
	
	ret = gpiod_line_settings_set_drive(settings_ouptut, GPIOD_LINE_DRIVE_PUSH_PULL);
	
	//ret = gpiod_line_settings_set_drive(settings_ouptut, GPIOD_LINE_DRIVE_OPEN_SOURCE);
	//ret = gpiod_line_settings_set_output_value(settings_ouptut, GPIOD_LINE_VALUE_ACTIVE);
	printf("drive %i \n", ret);

	line_cfg_out = gpiod_line_config_new();
	ret = gpiod_line_config_add_line_settings(line_cfg_out, &line_offset_out, 1,
						  settings_ouptut);
	printf("add_line_settings %i \n", ret);
	
	req_cfg = gpiod_request_config_new();
	if( req_cfg == NULL ) {
	  printf("Error req config gerneration failed \n");
	}
	  
	gpiod_request_config_set_consumer(req_cfg, "dddddde");
	
	request_loc = gpiod_chip_request_lines(chip, req_cfg, line_cfg_out);
	//request_loc = gpiod_chip_request_lines(chip, NULL, line_cfg_out);
	
	if( request_loc == NULL ) {
		printf("Error request is null pointer \n");
		printf("Error no %d \n", errno );
		return -1;
	}
	//usleep(100);

	ret = gpiod_line_request_set_value(request_loc, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);
	
	
	usleep(50);
	ret = gpiod_line_request_set_value(request_loc, line_offset_out, GPIOD_LINE_VALUE_INACTIVE);
	
	usleep(50);
	
	ret = gpiod_line_request_set_value(request_loc, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);
	
	usleep(100);
	
	//goto finish;



	// INPUT CONFIGS
	printf("set input configs");

	settings_input = gpiod_line_settings_new();
	ret = gpiod_line_settings_set_direction(settings_input, GPIOD_LINE_DIRECTION_INPUT);
	printf("direction %i \n", ret);
	//gpiod_line_settings_set_bias(settings_input, GPIOD_LINE_BIAS_PULL_UP);
	gpiod_line_settings_set_bias(settings_input, GPIOD_LINE_BIAS_DISABLED);
	printf("drive %i \n", ret);
	//ret = gpiod_line_settings_set_edge_detection(settings_input, GPIOD_LINE_EDGE_FALLING);
	ret = gpiod_line_settings_set_edge_detection(settings_input, GPIOD_LINE_EDGE_RISING);
	printf("edge detect %i \n", ret);

	line_cfg_in = gpiod_line_config_new();
	ret = gpiod_line_config_add_line_settings(line_cfg_in, &line_offset_in, 1,
						  settings_input);
	printf("add_line_settings line_cfg_in %i \n", ret);

	req_cfg_in = gpiod_request_config_new();
	if( req_cfg_in == NULL ) {
	  printf("Error req config gerneration failed \n");
	}	
	gpiod_request_config_set_consumer(req_cfg_in, "ddddd");

	
	//if(
	
	request_loc_in = gpiod_chip_request_lines(chip, req_cfg_in, line_cfg_in);
	
	if( request_loc_in == NULL ) {
		printf("Error request_loc_in is null pointer \n");
		printf("Error no %d \n", errno );
		return -1;
	}
        
        start = clock();
        clock_gettime(c_clk, &start_t);
        enum gpiod_line_value val = GPIOD_LINE_VALUE_ACTIVE;
        //enum gpiod_line_value val = GPIOD_LINE_VALUE_INACTIVE;
        //printf(" %i %i %i \n", (int) request_loc, (int) line_offset, (int) val);

	
	clock_gettime(c_clk, &now_t);
	printf("request_set_value %i \n", ret);
	printf("time %l \n",  now_t.tv_nsec - start_t.tv_nsec);
	
	
	settings_input_low = gpiod_line_settings_new();
	ret = gpiod_line_settings_set_direction(settings_input_low, GPIOD_LINE_DIRECTION_INPUT);
	printf("direction %i \n", ret);
	gpiod_line_settings_set_bias(settings_input_low, GPIOD_LINE_BIAS_DISABLED);
	printf("drive %i \n", ret);
	//ret = gpiod_line_settings_set_edge_detection(settings_input, GPIOD_LINE_EDGE_FALLING);
	//ret = gpiod_line_settings_set_edge_detection(settings_input_low, GPIOD_LINE_EDGE_RISING);
	printf("edge detect %i \n", ret);

	line_cfg_in_low = gpiod_line_config_new();
	//ret = gpiod_line_config_add_line_settings(line_cfg_in_low, &line_offset, 1,
	//					  settings_input);
	//printf("add_line_settings %i \n", ret);
	
	
	usleep(500);
	clock_gettime(c_clk, &start_t);
	//printf("INACTIVE time %i \n", clock()-start);
	val = GPIOD_LINE_VALUE_INACTIVE;
	ret = gpiod_line_request_set_value(request_loc, line_offset_out, val);
	//printf("gpiod_line_request_set_value %i \n", ret);
	//clock_gettime(c_clk, &now_t);
	//printf("INACTIVE end time %i \n", now_t.tv_nsec-start_t.tv_nsec);
	
	//wait time
	//clock_gettime(c_clk, &start_t);
	struct timespec dur;
	dur.tv_sec = 0;
	dur.tv_nsec = 500000;
	struct timespec remain;

	nanosleep(&dur, &remain);

	//clock_gettime(c_clk, &now_t);

	ret = gpiod_line_request_set_value(request_loc, line_offset_out, GPIOD_LINE_VALUE_ACTIVE);

	
	//WAIT edge events
	//ret = gpiod_line_request_wait_edge_events(request_loc, 100000000);
	
	ret = gpiod_line_request_wait_edge_events(request_loc_in, -1);
	
	clock_gettime(c_clk, &start_t);
	ret = gpiod_line_request_wait_edge_events(request_loc_in, -1);
	
	//ret = gpiod_line_request_read_edge_events(request_loc_in, event_buffer, 2);
	clock_gettime(c_clk, &now_t);
	printf("gpiod_line_request_read_edge_events ret %i \n", ret);
	
	//clock_gettime(c_clk, &now_t);
	printf("edge event  %ld \n",  now_t.tv_nsec - start_t.tv_nsec);
	//printf("gpiod_line_request_reconfigure_lines %i \n", ret);
	
	//printf("wait edge event ret %d \n", ret);
	//usleep(1);
	enum gpiod_line_value read = gpiod_line_request_get_value(request_loc_in, line_offset_in);
	printf("got value %i \n", read);
	printf("got GPIOD_LINE_VALUE_ACTIVE %i GPIOD_LINE_VALUE_INACTIVE %i \n", GPIOD_LINE_VALUE_ACTIVE, GPIOD_LINE_VALUE_INACTIVE);


	usleep(10);

	//char data[2] = { 0xCC, 0xBE };
	char data[1] = { 0x33 };
	write_cmd(request_loc, data, 1);

	usleep(100);
	char data_read[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	read_cmd(request_loc, request_loc_in, data_read, 8);

	char data_crc[7] = { data_read[0], data_read[1], data_read[2], data_read[3], data_read[4], data_read[5], data_read[6] };
	compute_crc(data_crc, 7);
	
	//reset(request_loc, request_loc_in);
	//CC(request_loc);
	
	//data[0] = 0x44; //copy scratchpad
	//write_cmd(request_loc, data, 1);
	
	//sleep(1);
	
	reset(request_loc, request_loc_in);
	usleep(100);
	CC(request_loc);
	usleep(300);
	
	data[0] = 0xBE; //read scratchpad
	//write_cmd(request_loc, data, 1);
	usleep(100);
	char data_scratchpad[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	//read_cmd(request_loc, request_loc_in, data_read, 9);
finish:
        gpiod_line_request_release(request_loc);
        gpiod_line_config_free(line_cfg_in);
        gpiod_line_config_free(line_cfg_out);
        
        gpiod_line_settings_free(settings_ouptut);
        gpiod_line_settings_free(settings_input);
        

	return EXIT_SUCCESS;


close_chip:
	gpiod_chip_close(chip);
	return -1;

}
