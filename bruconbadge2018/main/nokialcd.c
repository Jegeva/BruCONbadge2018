#include "./nokialcd.h"
#include "./brucon.h"
#include "./test.h"
#include "map.h"
#include "westvleteren.h"
#include "brucon_adc.h"
#include "brucon_nvs.h"

#include <freertos/task.h>
extern TickType_t  last_click;

#define USE_BITBANG 0

spi_device_handle_t spi;
uint8_t driver;

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;


DRAM_ATTR static const lcd_init_cmd_t Epson_init_cmds[]={
};

DRAM_ATTR static const lcd_init_cmd_t Phillips_init_cmds[]={
};


uint16_t * frame=NULL;// [ROW_LENGTH*COL_HEIGHT];


void do_spi_init();


#define CMD(x)  (x)
#define DATA(x) (0x100 | (x))

typedef struct {
    int bit;             /* next available bit position, MSB to LSB */
    int pos;             /* current position in buffer[] */
    int total_bits;      /* total bits currently in the buffer */
    int max_bits;        /* maximum number of bits in the buffer */
    uint32_t buffer[0];
} bit_buffer_t;

static inline void bit_buffer_clear(bit_buffer_t *bb) {
    bb->bit = 0;
    bb->pos = 0;
    bb->total_bits = 0;
    bb->buffer[0] = 0;   /* Only the first element of the buffer needs to be cleared, the 'next'
                          * element will be cleared when the buffer is getting used */
}

static inline bit_buffer_t *bit_buffer_alloc(int bits) {
    bit_buffer_t *bb = heap_caps_malloc(sizeof(bit_buffer_t) + (bits + 31) / 8, MALLOC_CAP_DMA);
    if (bb)
        bit_buffer_clear(bb);
    return bb;
}

static inline void bit_buffer_add(bit_buffer_t *bb, int bits, uint32_t data) {
    int available = 32 - bb->bit;  /* the available bits in buffer[pos] */

    if (bits == 0)
        return;

    if (available >= bits) {       /* test if it fits in one step */
        bb->buffer[bb->pos] |= __builtin_bswap32(data << (available - bits));
    }
    else {                         /* crossing the 32-bit boundary */
        bb->buffer[bb->pos] |= __builtin_bswap32(data >> (bits - available));
        bb->buffer[bb->pos + 1] = __builtin_bswap32(data << (32 - (bits - available)));
    }

    bb->total_bits += bits;
    bb->bit += bits;
    bb->pos += bb->bit / 32;
    bb->bit %= 32;

    if (bb->bit == 0)              /* an element was filled without overflow, make sure that */
        bb->buffer[bb->pos] = 0;   /* the next element is cleared */
}

void lcd_send_bit_buffer(spi_device_handle_t spi, bit_buffer_t *bb) {
    esp_err_t ret;
    spi_transaction_t t = {
        .length = bb->total_bits,
        .tx_buffer = bb->buffer,
    };

    ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
}


static bit_buffer_t *line_buffer;






void switchbacklight(int state){
    if(state)
        last_click=xTaskGetTickCount();
    if(esp_reset_reason() == ESP_RST_BROWNOUT){
      gpio_set_level(ENSCR_PIN,0);
      printf("has a brownout, not turning backlight on")	;
      return;
    }
    gpio_set_level(ENSCR_PIN,state);
}

#if USE_BITBANG

void lcd_send(unsigned char is_data,unsigned char data)
{
    char jj;
    LCD_CS_L;
    if(is_data)
        LCD_DIO_H;
    else
        LCD_DIO_L;
    clockit;
    for (jj = 0; jj < 8; jj++)
    {
        if ((data & 0x80) == 0x80){

            LCD_DIO_H;
        } else {

            LCD_DIO_L;
        }

        clockit;
        data<<=1;
    }
    LCD_CS_H;
}



void init_lcd(int type)
{
    // frame = (uint16_t*) malloc(ROW_LENGTH*COL_HEIGHT*sizeof(uint16_t));
    gpio_config_t io_conf;
    driver = type;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =  (1ULL<<LCD_SCK) |  (1ULL<<LCD_DIO) | (1ULL<<LCD_RST) |  (1ULL<<LCD_CS)|  (1ULL<<TRIGLA);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    TRIG;

    
    LCD_SCK_L;
    LCD_DIO_L;
    dl_us(10);
    LCD_CS_H;
    dl_us(10);
    LCD_RST_L;
    dl_us(200);
    LCD_SCK_H;
    LCD_DIO_H;
    LCD_RST_H;

     do_spi_init();


    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =  (1ULL<<ENSCR_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
     

}

#else
portMUX_TYPE mmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE * mux = &mmux;

static inline void send_9(uint16_t data){
    esp_err_t ret;
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
        .length = 9,                     //Command is 9 bits
    };
    *((uint32_t *)&t.tx_data) = SPI_SWAP_DATA_TX(data, 9);

    ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void lcd_send(char t,uint8_t d)
{
    uint16_t c = ((t<<8) | d);
    send_9(c);
}

void lcd_cmd(uint8_t cmd)
{
    uint16_t c = (0x000 | cmd);
    send_9(c);
}

void lcd_data(uint8_t data)
{
    uint16_t c = (0x100 | data);
    send_9(c);
}


void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    gpio_set_level(LCD_CS, 0);
}
void lcd_spi_post_transfer_callback(spi_transaction_t *t)
{
    gpio_set_level(LCD_CS, 1);
}


 

void init_lcd(int type)
{
    esp_err_t ret;
    //int i;
    vPortCPUInitializeMutex(mux);
    gpio_config_t io_conf;
    driver = type;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =  (1ULL<<LCD_SCK) |  (1ULL<<LCD_DIO) | (1ULL<<LCD_RST) |  (1ULL<<LCD_CS)|  (1ULL<<TRIGLA);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    TRIG;

    LCD_SCK_L;
    LCD_DIO_L;
    dl_us(10);
    LCD_CS_H;
    dl_us(10);
    LCD_RST_L;
    dl_us(200);
    LCD_SCK_H;
    LCD_DIO_H;
    LCD_RST_H;


    spi_bus_config_t buscfg={
        .mosi_io_num=LCD_DIO,
        .sclk_io_num=LCD_SCK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=30000000,               //Clock out at 1 MHz
        .mode=3,                                //SPI mode 3

        .queue_size=50,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=NULL,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .spics_io_num=LCD_CS,               //CS pin
        /* .pre_cb=lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
           .post_cb=lcd_spi_post_transfer_callback //Specify pre-transfer callback to handle D/C line*/
    };
    gpio_config_t io_conf2={
        .intr_type = GPIO_PIN_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<LCD_SCK) |  (1ULL<<LCD_DIO) |  (1ULL<<LCD_RST) |  (1ULL<<LCD_CS)|  (1ULL<<TRIGLA),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf2);

    driver = type;



    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    assert(ret==ESP_OK);
    do_spi_init();

    
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask =  (1ULL<<ENSCR_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1<<PIN_HEATER_ALC);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    line_buffer = bit_buffer_alloc(((ROW_LENGTH + 1) / 2 * 3) * 9);
}



#endif




void do_spi_init()
{
    TRIG;
    LCD_CS_H;
    dl_us(10);
    LCD_RST_L;
    dl_us(200);
    LCD_RST_H;

    if (driver == PHILLIPS){
        lcd_send(LCD_COMMAND,SLEEPOUT);	// Sleep Out (0x11)
        lcd_send(LCD_COMMAND,BSTRON);   	// Booster voltage on (0x03)
        lcd_send(LCD_COMMAND,DISPON);		// Display on (0x29)
        // lcd_send(LCD_COMMAND,INVON);		// Inversion on (0x20)
        // 12-bit color pixel format:
        lcd_send(LCD_COMMAND,PCOLMOD);		// Color interface format (0x3A)
        lcd_send(LCD_DATA,0x03);			// 0b011 is 12-bit/pixel mode
        lcd_send(LCD_COMMAND,MADCTL);		// Memory Access Control(PHILLIPS)

        lcd_send(LCD_DATA,0x00);
        lcd_send(LCD_COMMAND,SETCON);		// Set Contrast(PHILLIPS)
        lcd_send(LCD_DATA,0x3f);

        lcd_send(LCD_COMMAND,NOPP); // nop(PHILLIPS)
    }


}


void fillframe12B(int color_12b)
{
    uint32_t i =0;
    while(i<ROW_LENGTH*COL_HEIGHT)
            frame[i++]=color_12b;
}


void go_framep(uint16_t *p)
{
    bit_buffer_clear(line_buffer);
    bit_buffer_add(line_buffer, 9, CMD((driver == EPSON ? PASET : PASETP)));
    bit_buffer_add(line_buffer, 9, DATA(0));
    bit_buffer_add(line_buffer, 9, DATA(131));
    bit_buffer_add(line_buffer, 9, CMD((driver == EPSON ? CASET : CASETP)));
    bit_buffer_add(line_buffer, 9, DATA(0));
    bit_buffer_add(line_buffer, 9, DATA(131));
    bit_buffer_add(line_buffer, 9, CMD((driver == EPSON ? RAMWR : RAMWRP)));
    lcd_send_bit_buffer(spi, line_buffer);

    for (unsigned int y = 0; y < COL_HEIGHT; y++) {
        bit_buffer_clear(line_buffer);
        for (unsigned int x = 0; x < ROW_LENGTH; x += 2, p += 2) {
            bit_buffer_add(line_buffer, 9, 0x100 | (((*p) >> 4) & 0xff));
            bit_buffer_add(line_buffer, 9, 0x100 | ((*p & 0x0F) << 4) | (*(p + 1) >> 8));
            bit_buffer_add(line_buffer, 9, 0x100 | (*(p + 1) & 0xff));
        }
        lcd_send_bit_buffer(spi, line_buffer);
        bit_buffer_clear(line_buffer);
    }
}

void lcd_send_pixels(int p1, int p2)
{
    esp_err_t ret;
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA,
        .length = 27,
    };
    uint32_t data = 0;

    data |= (0x100 | ((p1>>4)&0x00FF)) << 18;
    data |= (0x100 | ((p1&0x0F)<<4)|(p2>>8)) << 9;
    data |= (0x100 | (p2&0x0FF));

    *((uint32_t *)&t.tx_data) = SPI_SWAP_DATA_TX(data, 27);

    ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void bruconlogo()
{
    go_framep(bruconbmp);
}

#define RAND_NR_STR 13
char * radnstr[RAND_NR_STR] = {
//  "Drink reponsibly"
    "drive",
    "dd",
    "sudo",
    "ssh 2 production",
    "devops",
    "update firmware",
    "write in c",
    "write in ASM",
    "write shellcodes",
    "heap feng shui",
    "heap spray",
    "0 day",
    "solder",
    
};



uint32_t VivaLaVodkaL(void* arg){
    preventbacklighttimeoutTask = 1;
    
    TaskHandle_t Tasktemp;
    int32_t score = getBruCONConfigUint32("AlcCal");
    gpio_config_t io_conf;
    char * tmp_str = (char*)calloc(50,sizeof(char));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1<<PIN_HEATER_ALC);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(PIN_HEATER_ALC,1);
    lcd_clearB12(0);
    lcd_setStr("DrinkResponsibly",2,0,B12_WHITE,0,1,0);
    lcd_setStr("  This is a TOY",18,0,B12_WHITE,0,1,0);
    lcd_setStr(" DO NOT TRUST IT",34,0,B12_RED,0,1,0);
    lcd_setStr(" Don't drink &",56,0,B12_WHITE,0,1,0);
    char * p =radnstr[esp_random()%RAND_NR_STR];
    lcd_setStr(p,72,64-((strlen(p)/2)*8),B12_WHITE,0,1,0);
    TickType_t tickstart = xTaskGetTickCount();
    while((xTaskGetTickCount() -  tickstart) < 2000 ){
        lcd_setStr(" Sensor heating",95,0,B12_RED,0,1,0);
        vTaskDelay(400 / portTICK_PERIOD_MS);
        lcd_setRect(90,0,108 ,131, 1, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);

    }
    tickstart = xTaskGetTickCount();
    getAlcSens=1;
    xTaskCreate( &  getAlcTask  , "getAlc" , 4096, NULL , 5| portPRIVILEGE_BIT , &Tasktemp );
    while((xTaskGetTickCount() -  tickstart) < 1500 ){
        lcd_setStr(" Blow on Sensor",90,0,B12_GREEN,0,1,0);
        vTaskDelay(400 / portTICK_PERIOD_MS);
        lcd_setRect(90,0,108 ,131, 1, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);

    }

    gpio_set_level(PIN_HEATER_ALC,0);
    while(getAlcSens){
      vTaskDelay(200);
    };
    vTaskDelete(Tasktemp);
    score = abs(Alc_level - score);
    if(score < 0)
      score=0;
    lcd_setRect(90,0,108 ,131, 1, 0);
    sprintf(tmp_str,"C2H6O lvl:%d",score);
    lcd_setStr(tmp_str,95,0,B12_WHITE,0,1,0);
    printf("ALC SCORE:%d\n",score);
    preventbacklighttimeoutTask = 0;
    last_click = xTaskGetTickCount();
    free(tmp_str);
    return score;
}

void MapWestL(void* arg){
    go_framep(westvleterenbmp);

};

void MapNovoL(void* arg){
    go_framep(mapbmp);
};



void lcd_clearB12(int color){
    bit_buffer_clear(line_buffer);
    bit_buffer_add(line_buffer, 9, CMD((driver == EPSON ? PASET : PASETP)));
    bit_buffer_add(line_buffer, 9, DATA(0));
    bit_buffer_add(line_buffer, 9, DATA(131));
    bit_buffer_add(line_buffer, 9, CMD((driver == EPSON ? CASET : CASETP)));
    bit_buffer_add(line_buffer, 9, DATA(0));
    bit_buffer_add(line_buffer, 9, DATA(131));
    bit_buffer_add(line_buffer, 9, CMD((driver == EPSON ? RAMWR : RAMWRP)));
    lcd_send_bit_buffer(spi, line_buffer);

    /* Prepare one row */
    bit_buffer_clear(line_buffer);
    for (unsigned int x = 0; x < ROW_LENGTH; x += 2) {
        bit_buffer_add(line_buffer, 9, 0x100 | (((color) >> 4) & 0xff));
        bit_buffer_add(line_buffer, 9, 0x100 | ((color & 0x0F) << 4) | (color >> 8));
        bit_buffer_add(line_buffer, 9, 0x100 | (color & 0xff));
    }

    /* Send the same row until the screen is filled */
    for (unsigned int y = 0; y < COL_HEIGHT; y++)
        lcd_send_bit_buffer(spi, line_buffer);
}

void lcd_contrast(char setting){
    	if (driver == EPSON)
	{
		setting &= 0x3F;	// 2 msb's not used, mask out
        lcd_send(LCD_COMMAND,VOLCTR);	// electronic volume, this is the contrast/brightness(EPSON)
		lcd_send(LCD_DATA,setting);	// volume (contrast) setting - course adjustment,  -- original was 24
		lcd_send(LCD_DATA,3);			// TODO: Make this coarse adjustment variable, 3's a good place to stay
	}
	else if (driver == PHILLIPS)
	{
		setting &= 0x7F;	// msb is not used, mask it out
        lcd_send(LCD_COMMAND,SETCON);	// contrast command (PHILLIPS)
		lcd_send(LCD_DATA,setting);	// volume (contrast) setting - course adjustment,  -- original was 24
	}

};
void lcd_setArc(int x0, int y0, int radius, int arcSegments[], int numSegments, int lineThickness, int color){};
void lcd_setCircle (int x0, int y0, int radius, int color, int lineThickness){};

void lcd_setChar(char c, int x, int y, int fColor, int bColor, char transp)
{
	y	=	(COL_HEIGHT - 1) - y; // make display "right" side up
	x	=	(ROW_LENGTH - 2) - x;

	int             i,j;
	unsigned int    nCols;
	unsigned int    nRows;
	unsigned int    nBytes;
	unsigned char   PixelRow;
	unsigned char   Mask;
	unsigned int    Word0;
	unsigned int    Word1;
	const unsigned char   *pFont;
	const unsigned char   *pChar;


	// get pointer to the beginning of the selected font table
	pFont = (const unsigned char *)FONT8x16;
	// get the nColumns, nRows and nBytes
	nCols = *(pFont);
	nRows = *(pFont + 1);
	nBytes = *(pFont + 2);
	// get pointer to the last byte of the desired character
	pChar = pFont + (nBytes * (c - 0x1F)) + nBytes - 1;

	if (driver)	// if it's an epson
	{
		// Row address set (command 0x2B)
		lcd_send(LCD_COMMAND,PASET);
		lcd_send(LCD_DATA,x);
		lcd_send(LCD_DATA,x + nRows - 1);
		// Column address set (command 0x2A)
		lcd_send(LCD_COMMAND,CASET);
		lcd_send(LCD_DATA,y);
		lcd_send(LCD_DATA,y + nCols - 1);

		// WRITE MEMORY
		lcd_send(LCD_COMMAND,RAMWR);
		// loop on each row, working backwards from the bottom to the top
		for (i = nRows - 1; i >= 0; i--) {
			// copy pixel row from font table and then decrement row
			PixelRow = *(pChar++);
			// loop on each pixel in the row (left to right)
			// Note: we do two pixels each loop
			Mask = 0x80;
			for (j = 0; j < nCols; j += 2)
			{
				// if pixel bit set, use foreground color; else use the background color
				// now get the pixel color for two successive pixels
				if ((PixelRow & Mask) == 0)
					Word0 = frame[i*ROW_LENGTH+j];
				else
					Word0 = fColor;
				Mask = Mask >> 1;
				if ((PixelRow & Mask) == 0)
					Word1 = bColor;
				else
					Word1 = fColor;
				Mask = Mask >> 1;
				// use this information to output three data bytes
				lcd_send_pixels(Word0, Word1);
			}
		}
	}
	else
	{
        // fColor = swapColors(fColor);
        // bColor = swapColors(bColor);

		// Row address set (command 0x2B)
		lcd_send(LCD_COMMAND,PASETP);
		lcd_send(LCD_DATA,x);
		lcd_send(LCD_DATA,x + nRows - 1);
		// Column address set (command 0x2A)
		lcd_send(LCD_COMMAND,CASETP);
		lcd_send(LCD_DATA,y);
		lcd_send(LCD_DATA,y + nCols - 1);

		// WRITE MEMORY
		lcd_send(LCD_COMMAND,RAMWRP);
		// loop on each row, working backwards from the bottom to the top
		pChar+=nBytes-1;  // stick pChar at the end of the row - gonna reverse print on phillips
		for (i = nRows - 1; i >= 0; i--) {
			// copy pixel row from font table and then decrement row
			PixelRow = *(pChar--);
			// loop on each pixel in the row (left to right)
			// Note: we do two pixels each loop
			Mask = 0x01;  // <- opposite of epson
			for (j = 0; j < nCols; j += 2)
			{
				// if pixel bit set, use foreground color; else use the background color
				// now get the pixel color for two successive pixels
				if ((PixelRow & Mask) == 0)
					Word0 = bColor;//frame[i*ROW_LENGTH+j];
				else
					Word0 = fColor;
				Mask = Mask << 1; // <- opposite of epson
				if ((PixelRow & Mask) == 0)
					Word1 = bColor;//frame[i*ROW_LENGTH+j];
				else
					Word1 = fColor;
				Mask = Mask << 1; // <- opposite of epson
				// use this information to output three data bytes
				lcd_send_pixels(Word0, Word1);
			}
		}
	}
}


void lcd_setStr(char *pString, int x, int y, int fColor, int bColor, char uselastfill, char newline)
{
	x = x + 12;
	y = y + 7;
    int originalY = y;

	// loop until null-terminator is seen
	while (*pString != 0x00) {
		// draw the character
		lcd_setChar(*pString++, x, y, fColor, bColor,uselastfill);
		// advance the y position
		y = y + 8;
		// bail out if y exceeds 131

		if ((y > 131) ) {
            if(newline){

            x = x + 16;
            y = originalY;
            } else {
                break;

            }

        }
        if (x > 131) break;
	}
}

void setPixel(int color, unsigned char x, unsigned char y)
{
        y       =       (COL_HEIGHT - 1) - y;
        x = (ROW_LENGTH - 1) - x;

        if (driver == EPSON) // if it's an epson
        {
                lcd_send(LCD_COMMAND,PASET);  // page start/end ram
                lcd_send(LCD_DATA,x);
                lcd_send(LCD_DATA,ENDPAGE);

                lcd_send(LCD_COMMAND,CASET);  // column start/end ram
                lcd_send(LCD_DATA,y);
                lcd_send(LCD_DATA,ENDCOL);

                lcd_send(LCD_COMMAND,RAMWR);  // write
                lcd_send(LCD_DATA,(color>>4)&0x00FF);
                lcd_send(LCD_DATA,((color&0x0F)<<4)|(color>>8));
                lcd_send(LCD_DATA,color&0x0FF);
        }
        else if (driver == PHILLIPS)  // otherwise it's a phillips
        {
                lcd_send(LCD_COMMAND,PASETP); // page start/end ram
                lcd_send(LCD_DATA,x);
                lcd_send(LCD_DATA,x);

                lcd_send(LCD_COMMAND,CASETP); // column start/end ram
                lcd_send(LCD_DATA,y);
                lcd_send(LCD_DATA,y);

                lcd_send(LCD_COMMAND,RAMWRP); // write

                lcd_send(LCD_DATA,(unsigned char)((color>>4)&0x00FF));
                lcd_send(LCD_DATA,(unsigned char)(((color&0x0F)<<4)|0x00));
        }
}

void lcd_setLine(int x0, int y0, int x1, int y1, int color)
{
        int dy = y1 - y0; // Difference between y0 and y1
        int dx = x1 - x0; // Difference between x0 and x1
        int stepx, stepy;

        if (dy < 0)
        {
                dy = -dy;
                stepy = -1;
        }
        else
                stepy = 1;

        if (dx < 0)
        {
                dx = -dx;
                stepx = -1;
        }
        else
                stepx = 1;

        dy <<= 1; // dy is now 2*dy
        dx <<= 1; // dx is now 2*dx
        setPixel(color, x0, y0);

        if (dx > dy)
        {
                int fraction = dy - (dx >> 1);
                while (x0 != x1)
                {
                        if (fraction >= 0)
                        {
                                y0 += stepy;
                                fraction -= dx;
                        }
                        x0 += stepx;
                        fraction += dy;
                        setPixel(color, x0, y0);
                }
        }
        else
        {
                int fraction = dx - (dy >> 1);
                while (y0 != y1)
                {
                        if (fraction >= 0)
                        {
                                x0 += stepx;
                                fraction -= dy;
                        }
                        y0 += stepy;
                        fraction += dx;
                        setPixel(color, x0, y0);
                }
        }
}

void lcd_setRect(int x0, int y0, int x1, int y1, unsigned char fill, int color)
{
    // check if the rectangle is to be filled
    unsigned int xstart,xend,ystart,yend,width=0,height=0;
    int j=0,i=0;
    // int tx0,tx1,ty0,ty1;

//    uint8_t cb0,cb1,cb2;

    if (fill == 1)
    {

        y0 = (COL_HEIGHT - 1) - y0;
        x0 = (ROW_LENGTH - 1) - x0;
        y1 = (COL_HEIGHT - 1) - y1;
        x1 = (ROW_LENGTH - 1) - x1;

        if(x0>x1){
            xstart=x1;
            xend=x0;
        } else {
            xstart=x0;
            xend=x1;
        }
        if(y0>y1){
            ystart=y1;
            yend=y0;
        } else {
            ystart=y0;
            yend=y1;
        }

        width  = xend - xstart;
        height = yend - ystart;


        if (driver == EPSON) // if it's an epson
        {
            i=0;

            lcd_send(LCD_COMMAND,PASET);  // page start/end ram
            lcd_send(LCD_DATA,xstart);
            lcd_send(LCD_DATA,xend);


            lcd_send(LCD_COMMAND,CASET);  // column start/end ram
            lcd_send(LCD_DATA,ystart);
            lcd_send(LCD_DATA,yend);
            j++;

            lcd_send(LCD_COMMAND,RAMWR);  // write

            while( i < (width*height)/2 ){
                lcd_send_pixels(color, color);
                i++;

            }
            if((width*height) & 1){
                lcd_send(LCD_DATA,((color>>4)&0x00FF));
                lcd_send(LCD_DATA,(((color&0x0F)<<4)|0x00));
            }
        }
        else if (driver == PHILLIPS)  // otherwise it's a phillips
        {
            i=0;
            lcd_send(LCD_COMMAND,PASETP); // page start/end ram
            lcd_send(LCD_DATA,xstart);
            lcd_send(LCD_DATA,xend);
            ;

            lcd_send(LCD_COMMAND,CASETP); // column start/end ram
            lcd_send(LCD_DATA,ystart);
            lcd_send(LCD_DATA,yend);
            j++;


            lcd_send(LCD_COMMAND,RAMWRP); // write

            while( i < (((width*height)/2)+ (height>width?height:width))  ){
                lcd_send_pixels(color, color);
                i++;

            }
            if((width*height) & 1){
                lcd_send(LCD_DATA,((color>>4)&0x00FF));
                lcd_send(LCD_DATA,(((color&0x0F)<<4)|0x00));
            }
        }
    }


    else
    {
        // best way to draw an unfilled rectangle is to draw four lines
        lcd_setLine(x0, y0, x1, y0, color);
        lcd_setLine(x0, y1, x1, y1, color);
        lcd_setLine(x0, y0, x0, y1, color);
        lcd_setLine(x1, y0, x1, y1, color);
    }

}


void lcd_printLogo(void){};
void lcd_printBMP(char * image_main){}; //prints an image (BMP);
void lcd_off(void){};
void lcd_on(void){};
