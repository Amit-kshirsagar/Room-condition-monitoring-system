#include <asf.h>
#include <stdio.h>
#include <conf_example.h>
#include <string.h> 
#include <avr/io.h>
#include <board.h>
#include <string.h>
#include <tc.h>
#include <adc_sensors.h>
#include <lightsensor.h>

//Temperature variables 
int8_t temp_counter=0;
int16_t current_temperature;
int16_t temp[6];
int32_t av_temp=0;
char Taverage[32];
//Light sensor variables 
int8_t light_counter=0;
int16_t current_light;
int16_t light[6];
uint32_t av_light=0;
char Laverage[32];
//Timer Variables 
uint8_t Counter10s=0;
//ADC temperature
struct adc_config adc_conft;
struct adc_channel_config adcch_conft;
//ADC light
struct adc_config adc_confl;
struct adc_channel_config adcch_confl;

static void adc_temp_handler(ADC_t *adc, uint8_t ch_mask, adc_result_t result)
{
	result=result >> 4;					//Only the upper 12 BITS of RES register are used (it gives the input value) 
	result=result & 0x07ff;				//MAX result from temp sensor taht is possible is 2047 i.e. only 11 BITS are required hence, the MSB is masked
	
	// Compute current temperature in Celsius, based on linearization( given in the manual)
	if (result > 697) 
	{ current_temperature = (int16_t)((-0.0295 * result) + 40.50229); }
	else if (result > 420) 
	{ current_temperature = (int16_t)((-0.04748 * result) + 53.25728); }
	else
	{ current_temperature = (int16_t)((-0.07776 * result) + 65.513); }

	temp[temp_counter]=current_temperature;		//the current temperature value is stored in the array
	PORTE_OUT=10;								// output is given on port E to check whether this function is being called or not
	temp_counter++;
	
}

void adc_temp_initialization(void)
{
	// Initialize configuration structures
	adc_read_configuration(&ADCA, &adc_conft);
	adcch_read_configuration(&ADCA, ADC_CH0, &adcch_conft);
	/* Configure the ADC module:
	 * - unsigned, 12-bit results
	 * - VCC voltage reference
	 * - 200 kHz maximum clock rate
	 * - manual conversion triggering
	 * - temperature sensor enabled
	 * - callback function
	 */
	adc_set_conversion_parameters(&adc_conft, ADC_SIGN_ON, ADC_RES_12,ADC_REF_VCC);
	adc_set_clock_rate(&adc_conft, 200000UL);			
	adc_set_conversion_trigger(&adc_conft, ADC_TRIG_MANUAL, 0, 0);
	adc_enable_internal_input(&adc_conft, ADC_INT_TEMPSENSE);
	adc_write_configuration(&ADCA, &adc_conft);
	adc_set_callback(&ADCA, &adc_temp_handler);
	/* Configure ADCA channel 0:
	 * - single-ended measurement from temperature sensor
	 * - interrupt flag set on completed conversion
	 * - interrupts disabled
	 */
	adcch_set_input(&adcch_conft, ADCCH_POS_PIN1, ADCCH_NEG_NONE,1);
	adcch_set_interrupt_mode(&adcch_conft, ADCCH_MODE_COMPLETE);
	adcch_enable_interrupt(&adcch_conft);
	adcch_write_configuration(&ADCA, ADC_CH0, &adcch_conft);
	// Enable the ADCA 
	adc_enable(&ADCA);
	
}
static void adc_light_handler(ADC_t *adc, uint8_t ch_mask, adc_result_t result)
{
	result=result >> 4;						//Only the upper 12 BITS of RES register are used (it gives the input value)  
	current_light=(int16_t)result;			
	/* normalizing the current light reading
	 *if light intensity is 0(low) current light reading =4095 and hence % intensity =0%
	 *if light intensity is max (high) current light reading =0 and hence % intensity =100%
	 */
	
	current_light=(4095-current_light)/4095;	
	current_light=current_light*100;
	light[light_counter]=current_light;			//storing the current light value in array
	light_counter++;
	
}

void adc_light_initialization(void)
{
	// Initialize configuration structures
	adc_read_configuration(&ADCB, &adc_confl);
	adcch_read_configuration(&ADCB, ADC_CH1, &adcch_confl);
	/* Configure the ADC module:
	 * - unsigned, 12-bit results
	 * - VCC voltage reference
	 * - 200 kHz maximum clock rate
	 * - manual conversion triggering
	 * - temperature sensor enabled
	 * - callback function
	 */
	adc_set_conversion_parameters(&adc_confl, ADC_SIGN_ON, ADC_RES_12,ADC_REF_VCC);
	adc_set_clock_rate(&adc_confl, 200000UL);			
	adc_set_conversion_trigger(&adc_confl, ADC_TRIG_MANUAL, 0, 0);
	adc_enable_internal_input(&adc_confl, ADC_INT_TEMPSENSE);
	adc_write_configuration(&ADCB, &adc_confl);
	adc_set_callback(&ADCB, &adc_light_handler);
	/* Configure ADCB channel 1:
	 * - single-ended measurement from temperature sensor
	 * - interrupt flag set on completed conversion
	 * - interrupts disabled
	 */
	adcch_set_input(&adcch_confl, ADCCH_POS_PIN0, ADCCH_NEG_NONE,1);
	adcch_set_interrupt_mode(&adcch_confl, ADCCH_MODE_COMPLETE);
	adcch_enable_interrupt(&adcch_confl);
	adcch_write_configuration(&ADCB, ADC_CH1, &adcch_confl);
	// Enable the ADCB 
	adc_enable(&ADCB);
	
}
static void timer_callback(void)
{
	Counter10s++;									// Counter which counts multiples of 10
	adc_start_conversion(&ADCA, ADC_CH0);			// Start adc conversion for temperature sensor
	adc_start_conversion(&ADCB, ADC_CH1);			// Start adc conversion for light sensor
	if(Counter10s>6)
	{
		Counter10s=0;								// Once 60 seconds i.e 1 minute is completed calculate the average of temperature and light values
		temp_counter=0;
		av_temp=temp[0]+temp[1]+temp[2]+temp[3]+temp[4]+temp[5];
		av_temp=av_temp/6;
		//sprintf(Taverage,32,"%s\r\n",av_temp);				 
		//gfx_mono_draw_string(Taverage, 0, 0, &sysfont);		//display av temp on lcd
		light_counter=0;
		av_light=light[0]+light[1]+light[2]+light[3]+light[4]+light[5];
		av_light=av_light/6;
		//sprintf(Laverage,32,"%s\r\n",av_light);
		//gfx_mono_draw_string(Laverage, 1, 0, &sysfont);		//display av light% on lcd
		PORTE_OUT=av_temp;					// output is given on port E to check whether this function is being called or not
	} 
	//PORTE_OUT=2;							// output is given on port E to check whether this function is being called or not
}

void sys_initialization(void)
{
	board_init();
	sysclk_init();
	pmic_init();
	gfx_mono_init();
	cpu_irq_enable();
	sleepmgr_init();
	adc_temp_initialization();
	adc_light_initialization();
}


int main(void)
{
	
	sys_initialization();
	PORTE_DIRSET=0xff;
	tc_enable(&TCC0);							// TCC0 selected
	tc_set_overflow_interrupt_callback(&TCC0,timer_callback); //SET UP THE INTERRUPT CALL BACK FUNCTION
	tc_set_wgm(&TCC0, TC_WG_NORMAL);			// SETS THE TC IN NORMAL MODE
	//tc_write_period(&TCC0,9766);				// this along with clk div 1024 will cause the timer to call interrupt after 10 seconds 
	tc_write_period(&TCC0,1);					// we have used this period for only simulation purpose, the timer will call interrupt after 1024 micro seconds
	tc_set_overflow_interrupt_level(&TCC0, TC_INT_LVL_LO);
	tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1024_gc); 
	PORTE_OUT=1;								// output is given on port E to check whether this function is being called or not
	do {
		
		//Sleep until ADC interrupt triggers
		//sleepmgr_enter_sleep();
	} while (1);

}