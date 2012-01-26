#include "osal.h" 
#include "ismd_core.h" 
#include "ismd_global_defs.h" 
#include "ismd_audio_defs.h" 
#include "ismd_audio.h" 
 
 
#define AUDIO_SADIV_SPDIF_48KHZ    6 
#define AUDIO_SADIV_I2S    1
#define READ_SIZE 8*1024 
#define NOT_TIMED 0 
 
 
static ismd_dev_t audio_dev_handle; 
static ismd_audio_processor_t audio_processor; 
static ismd_audio_output_t audio_output_handle; 
static ismd_port_handle_t audio_input_port_handle; 
static ismd_port_handle_t audio_output_port_handle; 
static os_thread_t input_thread; 
static int read_complete = 0; 
FILE *fp; 
 
 
ismd_result_t init_audio_driver(void); 
ismd_result_t free_audio_resources(void); 
void *input_feeder(void *arg); 
 
int main (void) { 
 
   ismd_result_t ismd_ret; 
    
   fp = fopen("/input.mp3", "rb"); 
   if (fp == 0) { 
      OS_INFO("could not open input"); 
      OS_ASSERT(0); 
   }  
 
   ismd_ret = init_audio_driver(); 
   if (ismd_ret != ISMD_SUCCESS) { 
      printf("init_audio_driver failed \n"); 
      assert(0); 
   } 
    
   os_thread_create(&input_thread, input_feeder, NULL, 0, 0, "input thread"); 
 
   while (!read_complete) { 
      os_sleep(20); 
   } 
 
   ismd_ret = free_audio_resources(); 
   if (ismd_ret != ISMD_SUCCESS) { 
      printf("free_audio_resources failed \n"); 
      assert(0); 
   } 
 
   os_thread_destroy(&input_thread); 
 
   fclose(fp); 
    
   return 0; 
} 
 
ismd_result_t 
init_audio_driver(void) { 
   ismd_result_t ismd_ret; 
   ismd_audio_output_config_t output_config; 
    
   ismd_ret = ismd_audio_open_processor(&audio_processor); 
   if (ismd_ret) { 
      printf("ismd_audio_open_processor failed\n"); 
      assert(0); 
   } 
   ismd_ret = ismd_audio_add_input_port(audio_processor, NOT_TIMED, &audio_dev_handle, 
&audio_input_port_handle); 
   if (ismd_ret) { 
      printf("ismd_audio_add_input_port failed\n"); 
      assert(0); 
   } 
   //Example hard coded for MPEG now, example extends for other algorithms 
   ismd_ret = ismd_audio_input_set_data_format(audio_processor, audio_dev_handle,  
ISMD_AUDIO_MEDIA_FMT_MPEG); 
   if (ismd_ret) { 
      printf("ismd_audio_input_set_data_format failed\n"); 
      assert(0); 
   }  
 
   //if decoder specific parameters are needed, this would be a good spot for that 
    
   ismd_ret = ismd_audio_input_enable(audio_processor, audio_dev_handle); 
   if (ismd_ret) { 
      printf("ismd_audio_input_enable failed\n"); 
      assert(0); 
   }  
 
   output_config.ch_config = ISMD_AUDIO_STEREO; 
   output_config.out_mode = ISMD_AUDIO_OUTPUT_PCM; 
   output_config.sample_rate = 48000; 
   output_config.sample_size = 16;
   output_config.stream_delay = 0; 
 
   ismd_ret = ismd_audio_add_phys_output(audio_processor, GEN3_HW_OUTPUT_SPDIF,
output_config, &audio_output_port_handle); 
   if (ismd_ret) { 
      printf("ismd_audio_add_phys_output failed\n"); 
      assert(0); 
   }    

/*
   ismd_ret = ismd_audio_output_set_external_bit_clock_div(audio_processor, 
audio_output_port_handle, AUDIO_SADIV_I2S); 
*/

   ismd_ret = ismd_audio_configure_master_clock(audio_processor, 36864000, ISMD_AUDIO_CLK_SRC_EXTERNAL);
   if (ismd_ret) { 
      printf("ismd_audio_add_phys_output failed\n"); 
      assert(0); 
   }   
 
   ismd_ret = ismd_audio_output_enable(audio_processor, audio_output_handle); 
   if (ismd_ret) { 
      printf("ismd_audio_output_enable failed\n"); 
      assert(0); 
   } 
 
   ismd_ret = ismd_dev_set_state(audio_dev_handle, ISMD_DEV_STATE_PLAY); 
   if (ismd_ret) { 
      printf("ismd_dev_set_state failed\n"); 
      assert(0); 
   }      
 
   return ISMD_SUCCESS; 
} 
 
ismd_result_t free_audio_resources(void) 
{ 
   ismd_result_t ismd_ret; 
    
   ismd_ret = ismd_dev_set_state(audio_dev_handle, ISMD_DEV_STATE_STOP); 
   if (ismd_ret) { 
      printf("ismd_dev_set_state failed\n"); 
      assert(0); 
   } 
 
   ismd_ret = ismd_dev_flush(audio_dev_handle); 
   if (ismd_ret) { 
      printf("ismd_dev_flush failed\n"); 
      assert(0); 
   }  
 
   ismd_ret = ismd_audio_output_disable(audio_processor, audio_output_handle); 
   if (ismd_ret) { 
      printf("ismd_audio_output_disable failed\n"); 
      assert(0); 
   } 
 
   ismd_ret = ismd_audio_remove_output(audio_processor, audio_output_handle); 
   if (ismd_ret) { 
      printf("ismd_audio_remove_input failed\n"); 
      assert(0); 
   } 
 
   ismd_ret = ismd_dev_close(audio_dev_handle); 
   if (ismd_ret) { 
      printf("ismd_dev_close failed\n"); 
      assert(0); 
   }   
 
   return ISMD_SUCCESS; 
} 
 
 
 
void * 
input_feeder(void *arg) 
{ 
char buf[READ_SIZE]; 
   ismd_buffer_handle_t ismd_buffer_handle; 
   ismd_result_t smd_ret; 
   ismd_buffer_descriptor_t ismd_buf_desc; 
   uint8_t *buf_ptr; 
 
   buf_ptr = (uint8_t*) arg; //only for compile reasons 
 
   /* More elegant reading and writing mechanisms available, just made simple as this 
is not target of this example*/ 
   while (fread(buf, 1, READ_SIZE, fp) == READ_SIZE) { 
      while (ismd_buffer_alloc(READ_SIZE, &ismd_buffer_handle) != ISMD_SUCCESS) { 
         OS_INFO("trying to get buffer\n"); 
         os_sleep(20); 
      } 
 
      smd_ret = ismd_buffer_read_desc(ismd_buffer_handle, &ismd_buf_desc); 
      if (smd_ret != ISMD_SUCCESS) { 
         OS_ASSERT(0); 
      } 
 
      buf_ptr = (uint8_t 
*)OS_MAP_IO_TO_MEM_NOCACHE(ismd_buf_desc.phys.base,ismd_buf_desc.phys.size); 
 
      memcpy(buf_ptr, buf, READ_SIZE); 
 
      ismd_buf_desc.phys.level = READ_SIZE; 
 
      smd_ret = ismd_buffer_update_desc(ismd_buffer_handle, &ismd_buf_desc); 
      if (smd_ret != ISMD_SUCCESS) { 
         OS_ASSERT(0); 
      } 
 
      OS_UNMAP_IO_FROM_MEM(buf_ptr,READ_SIZE); 
 
      while (ismd_port_write(audio_input_port_handle, ismd_buffer_handle) != 
ISMD_SUCCESS) { 
         os_sleep(20); 
      } 
 
   } 
 
   read_complete = 1; 
   OS_INFO("Done writing file in\n"); 
 
   return NULL; 
 
} 
