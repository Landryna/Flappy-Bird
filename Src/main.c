/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 ** This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * COPYRIGHT(c) 2019 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "st7735.h"
#include "lcd.h"
#include "fonts.h"
#include "testimg.h"

volatile uint8_t BUFF_SIZE = 127;

char Buff_Rx[127] = { '\0' };
char Buff_Tx[127] = { '\0' };

volatile uint8_t Busy_Tx = 0;
volatile uint8_t Empty_Tx = 0;
volatile uint8_t Busy_Rx = 0;
volatile uint8_t Empty_Rx = 0;
volatile int timer = 0;
volatile int poziom = 0;
volatile int wynikgry;

char Bx[127] = { '\0' };
volatile uint8_t Empty_Bx = 0;
volatile uint8_t koniec = 0;
volatile uint16_t suma = 0;
volatile uint16_t checksum = 0;
volatile uint8_t stan = 1;
volatile int odkodowanie;
volatile int zakres=1;
volatile int current;

volatile int gra = 0;
char BBx[127];

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
void zerujBx() {

	for (int i = 0; i < 128; i++) {
		Bx[i] = '\0';
	}

	Empty_Bx = 0;
	koniec = 0;

}
void zerujBBx() {

	for (int i = 0; i < 128; i++) {
		BBx[i] = '\0';
	}

}
void USART_fsend(char* format, ...) {
	char tmp_rs[128];
	int i;
	__IO int idx;
	va_list valist;
	va_start(valist, format);
	vsprintf(tmp_rs, format, valist);
	va_end(valist);
	idx = Empty_Tx;
	for (i = 0; i < strlen(tmp_rs); i++) {
		Buff_Tx[idx] = tmp_rs[i];
		idx++;
		if (idx >= BUFF_SIZE)
			idx = 0;
	}
	__disable_irq();

	if ((Empty_Tx == Busy_Tx)
			&& (__HAL_UART_GET_FLAG(&huart2,UART_FLAG_TXE) == SET)) {

		Empty_Tx = idx;
		uint8_t tmp = Buff_Tx[Busy_Tx];
		Busy_Tx++;
		if (Busy_Tx >= BUFF_SIZE)
			Busy_Tx = 0;
		HAL_UART_Transmit_IT(&huart2, &tmp, 1);

	} else {
		Empty_Tx = idx;
	}
	__enable_irq();
}

void pack(char* format, ...) {


	 uint16_t suma = 0;


	char tmp_rs[128];
	int i;
	__IO int idx = 0;
	va_list valist;
	va_start(valist, format);
	vsprintf(tmp_rs, format, valist);
	va_end(valist);

	BBx[idx] = 0xE1;
	idx++;



	for (i = 0; i < strlen(tmp_rs); i++) {

		if (tmp_rs[i] == 0xE1) {

			BBx[idx] = 0xE3;
			suma += 0xE3;
			idx++;

			BBx[idx] = 0xE4;
			suma += 0xE4;
			idx++;



		} else if (tmp_rs[i] == 0xE2) {

			BBx[idx] = 0xE3;
			suma += 0xE3;
			idx++;

			BBx[idx] = 0xE5;
			suma += 0xE5;
			idx++;




		} else if (tmp_rs[i] == 0xE3) {

			BBx[idx] = 0xE3;
			suma += 0xE3;
			idx++;

			BBx[idx] = 0xE6;
			suma += 0xE6;
			idx++;



		}else{

			BBx[idx] = tmp_rs[i];
		suma += tmp_rs[i];
		idx++;

		}
	}
	suma+=0x03;


	BBx[idx]= 0x02;
	idx++;

	BBx[idx]= 0x01;
	idx++;

	BBx[idx]= suma;
	idx++;

	BBx[idx]= 0xE2;
	idx++;

	BBx[idx]='\0';

 USART_fsend(BBx);

	zerujBBx();

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2) {
		Empty_Rx++;
		if (Empty_Rx >= BUFF_SIZE)
			Empty_Rx = 0;
		HAL_UART_Receive_IT(&huart2, (uint8_t*) &Buff_Rx[Empty_Rx], 1);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2) {
		if (Empty_Tx != Busy_Tx) {
			uint8_t tmp = Buff_Tx[Busy_Tx];
			Busy_Tx++;
			if (Busy_Tx >= BUFF_SIZE)
				Busy_Tx = 0;
			HAL_UART_Transmit_IT(&huart2, &tmp, 1);
		}

	}
}

uint8_t czypustyRx() {
	if (Empty_Rx == Busy_Rx) {
		return 0;
	} else {
		return 1;
	}
}

uint8_t USART_getchar() {

	uint8_t tmp;

	if (Empty_Rx != Busy_Rx) {

		tmp = Buff_Rx[Busy_Rx];
		Busy_Rx++;
		if (Busy_Rx >= BUFF_SIZE)
			Busy_Rx = 0;
		return tmp;
	} else
		return 0;
}

void odkoduj() {

	while (czypustyRx()) {
		uint8_t c;
		c = USART_getchar();

		switch (stan) {
		case 1:
			if (c == 0xE1) {
				stan = 2;
			}
			break;
		case 2:

			if (c == 0xE3) {
				checksum += c;
				stan = 3;
				break;
			} else if (c == 0xE2) {
				koniec = Empty_Bx;
				suma = Bx[koniec - 1];
				stan = 1;
				odkodowanie = 1;
				break;
			} else {
				checksum += c;
				Bx[Empty_Bx] = c;
				Empty_Bx++;
				if (Empty_Bx >= BUFF_SIZE)
				{
					stan = 1;
					odkodowanie = 1;
					zakres=0;
				}
				break;
			}

		case 3:
			if (c == 0xE4) {
				checksum += c;
				Bx[Empty_Bx] = 0xE1;
				Empty_Bx++;
				if (Empty_Bx >= BUFF_SIZE)
					{

					stan=1;
					odkodowanie =1;
					zakres=0;
					}
				else stan = 2;

			} else if (c == 0xE5) {
				checksum += c;
				Bx[Empty_Bx] = 0xE2;
				Empty_Bx++;
				if (Empty_Bx >= BUFF_SIZE)
				{

									stan=1;
									odkodowanie =1;
									zakres=0;
									}
								else stan = 2;

			} else if (c == 0xE6) {
				checksum += c;
				Bx[Empty_Bx] = 0xE3;
				Empty_Bx++;
				if (Empty_Bx >= BUFF_SIZE)
				{

									stan=1;
									odkodowanie=1;
									zakres=0;
									}
								else stan = 2;

			} else {
				stan = 1;
				zerujBx();

			}
			break;

		}
	}

}

uint8_t sprawdzsume() {

	checksum -= Bx[koniec - 1];
	checksum = checksum % 256; //aby mozna bylo zapisac sume kontrolna w HEX, przy duzych liczbach

	if (suma == checksum) {
		Bx[koniec - 1] = '\0';
		checksum = 0;
		return 1;
	} else {
		checksum = 0;
		return 0;
	}
}

uint8_t sprawdzadresy() {

	if (Bx[koniec - 3] == 0x01 && Bx[koniec - 2] == 0x02) {
		Bx[koniec - 3] = '\0';

		return 1;
	} else {
		return 0;
	}

}

int start() {

	ST7735_FillScreen(ST7735_BLACK);
	for (int x = 0; x < ST7735_WIDTH; x++) {
		ST7735_DrawPixel(x, 0, ST7735_WHITE);
		ST7735_DrawPixel(x, ST7735_HEIGHT - 1, ST7735_WHITE);
	}

	for (int y = 0; y < ST7735_HEIGHT; y++) {
		ST7735_DrawPixel(0, y, ST7735_WHITE);
		ST7735_DrawPixel(ST7735_WIDTH - 1, y, ST7735_WHITE);
	}

	ST7735_WriteString(33, 60, "FLAPPY", Font_11x18, ST7735_GREEN,
			ST7735_BLACK);
	ST7735_WriteString(44, 80, "BIRD", Font_11x18, ST7735_CYAN, ST7735_BLACK);

	for (int x = 3; x < 125; x++) {
		ST7735_FillRectangle(x, 140, 3, 3, ST7735_MAGENTA);
	}
	if (poziom == 0) {

		ST7735_WriteString(5, 102, "Press button:PLAY", Font_7x10, ST7735_CYAN,
				ST7735_BLACK);

	}
	if (poziom == 1) {
		ST7735_WriteString(5, 102, "Press button:LVL2", Font_7x10, ST7735_CYAN,
				ST7735_BLACK);
	}
	if (poziom == 2) {
		ST7735_WriteString(5, 102, "Press button:LVL3", Font_7x10, ST7735_CYAN,
				ST7735_BLACK);
	}
	if (poziom == 3) {
		ST7735_WriteString(16, 102, "!!!!WINNER!!!!", Font_7x10, ST7735_CYAN,
				ST7735_BLACK);
	}
	current = 1;
	gra = 1;

	return 1;

}
int stop() {

	ST7735_FillScreen(ST7735_BLACK);

	current=2;
	gra = 2;

	return 1;

}
int lvl() {
	ST7735_FillScreen(ST7735_BLACK);
	for (int x = 0; x < ST7735_WIDTH; x++) {
		ST7735_DrawPixel(x, 0, ST7735_RED);
		ST7735_DrawPixel(x, ST7735_HEIGHT - 1, ST7735_RED);
	}

	for (int y = 0; y < ST7735_HEIGHT; y++) {
		ST7735_DrawPixel(0, y, ST7735_RED);
		ST7735_DrawPixel(ST7735_WIDTH - 1, y, ST7735_RED);
	}

	//gora
	for (int y = 0; y < 100; y++) {
		ST7735_FillRectangle(50, y, 4, 4, ST7735_RED);

	}
	for (int y = 0; y < 110; y++) {
		ST7735_FillRectangle(65, y, 4, 4, ST7735_RED);

	}
	for (int y = 0; y < 80; y++) {
		ST7735_FillRectangle(80, y, 4, 4, ST7735_RED);

	}
	//dol
	for (int y = 160; y > 80; y--) {
		ST7735_FillRectangle(100, y, 4, 4, ST7735_RED);

	}
	for (int y = 160; y > 130; y--) {
		ST7735_FillRectangle(50, y, 4, 4, ST7735_RED);

	}
	for (int y = 160; y > 140; y--) {
		ST7735_FillRectangle(65, y, 4, 4, ST7735_RED);

	}
	current = 3;
	gra = 3;

	return 1;
}

int lvl2() {
	ST7735_FillScreen(ST7735_BLACK);
	for (int x = 0; x < ST7735_WIDTH; x++) {
		ST7735_DrawPixel(x, 0, ST7735_BLUE);
		ST7735_DrawPixel(x, ST7735_HEIGHT - 1, ST7735_BLUE);
	}

	for (int y = 0; y < ST7735_HEIGHT; y++) {
		ST7735_DrawPixel(0, y, ST7735_BLUE);
		ST7735_DrawPixel(ST7735_WIDTH - 1, y, ST7735_BLUE);
	}

	for (int y = 0; y < 95; y++) {
		ST7735_FillRectangle(30, y, 4, 4, ST7735_BLUE);

	}
	for (int y = 0; y < 80; y++) {
		ST7735_FillRectangle(50, y, 4, 4, ST7735_BLUE);

	}
	for (int y = 0; y < 120; y++) {
		ST7735_FillRectangle(110, y, 4, 4, ST7735_BLUE);

	}

	//dol
	for (int y = 160; y > 115; y--) {
		ST7735_FillRectangle(30, y, 4, 4, ST7735_BLUE);

	}
	for (int y = 160; y > 100; y--) {
		ST7735_FillRectangle(50, y, 4, 4, ST7735_BLUE);

	}

	for (int y = 160; y > 145; y--) {
		ST7735_FillRectangle(110, y, 4, 4, ST7735_BLUE);

	}
	current=4;
	gra = 4;

	return 1;
}

int lvl3() {
	ST7735_FillScreen(ST7735_BLACK);
	for (int x = 0; x < ST7735_WIDTH; x++) {
		ST7735_DrawPixel(x, 0, ST7735_GREEN);
		ST7735_DrawPixel(x, ST7735_HEIGHT - 1, ST7735_GREEN);
	}

	for (int y = 0; y < ST7735_HEIGHT; y++) {
		ST7735_DrawPixel(0, y, ST7735_GREEN);
		ST7735_DrawPixel(ST7735_WIDTH - 1, y, ST7735_GREEN);
	}

	for (int y = 0; y < 100; y++) {
		ST7735_FillRectangle(30, y, 4, 4, ST7735_GREEN);
	}
	for (int y = 0; y < 90; y++) {
		ST7735_FillRectangle(45, y, 4, 4, ST7735_GREEN);
	}
	for (int y = 0; y < 80; y++) {
		ST7735_FillRectangle(60, y, 4, 4, ST7735_GREEN);
	}
	for (int y = 0; y < 70; y++) {
		ST7735_FillRectangle(75, y, 4, 4, ST7735_GREEN);
	}
	for (int y = 0; y < 60; y++) {
		ST7735_FillRectangle(90, y, 4, 4, ST7735_GREEN);
	}
	for (int y = 0; y < 50; y++) {
		ST7735_FillRectangle(105, y, 4, 4, ST7735_GREEN);
	}

	//dol

	for (int y = 160; y > 120; y--) {
		ST7735_FillRectangle(30, y, 4, 4, ST7735_GREEN);

	}
	for (int y = 160; y > 110; y--) {
		ST7735_FillRectangle(45, y, 4, 4, ST7735_GREEN);

	}
	for (int y = 160; y > 100; y--) {
		ST7735_FillRectangle(60, y, 4, 4, ST7735_GREEN);

	}
	for (int y = 160; y > 90; y--) {
		ST7735_FillRectangle(75, y, 4, 4, ST7735_GREEN);

	}
	for (int y = 160; y > 80; y--) {
		ST7735_FillRectangle(90, y, 4, 4, ST7735_GREEN);

	}
	for (int y = 160; y > 70; y--) {
		ST7735_FillRectangle(105, y, 4, 4, ST7735_GREEN);

	}
	current=5;
	gra = 5;
	return 1;
}

int wynik() {

	ST7735_FillScreen(ST7735_BLACK);
	ST7735_WriteString(20, 70, "GAME OVER", Font_11x18, ST7735_YELLOW,
			ST7735_BLACK);
	if (wynikgry == 1) {
		ST7735_WriteString(16, 90, "Pathetic", Font_7x10, ST7735_YELLOW,
				ST7735_BLACK);
		ST7735_WriteString(36, 115, "Score: 0", Font_7x10, ST7735_CYAN,
						ST7735_BLACK);
	}
	if (wynikgry == 2) {
		ST7735_WriteString(16, 90, "Not so bad", Font_7x10, ST7735_YELLOW,
				ST7735_BLACK);
		ST7735_WriteString(36, 115, "Score: 1", Font_7x10, ST7735_CYAN,
								ST7735_BLACK);
	}

	if (wynikgry == 3) {
		ST7735_WriteString(16, 90, "Almost...", Font_7x10, ST7735_YELLOW,
				ST7735_BLACK);
		ST7735_WriteString(36, 115, "Score: 2", Font_7x10, ST7735_CYAN,
									ST7735_BLACK);
	}
	current=6;
	return 1;
}
void komunikat()

{
	odkoduj();


	if (odkodowanie) {
		odkodowanie = 0;
		if(zakres)
		{

		if (sprawdzsume()) {

			if (sprawdzadresy()) {
				if (strcmp("ON", Bx) == 0) {
					pack("ON");
					poziom = 0;
					zerujBx();
					start();

				} else if (strcmp("OFF", Bx) == 0) {
					pack("OFF");

					zerujBx();
					stop();

				} else if (strcmp("PLAY1", Bx) == 0) {
					pack("PLAY1");
					zerujBx();

					lvl();

				} else if (strcmp("PLAY2", Bx) == 0) {
					pack("PLAY2");
					zerujBx();
					gra = 4;
					lvl2();

				} else if (strcmp("PLAY3", Bx) == 0) {
					pack("PLAY3");
					zerujBx();
					gra = 5;
					lvl3();

				}
				else if (strcmp("A", Bx) == 0) {
					switch(current)
					{
					case 1:
						pack("ON");
						zerujBx();
						break;
					case 2:
						pack("OFF");
						zerujBx();
						break;
					case 3:
						pack("PLAY1");
						zerujBx();
						break;
					case 4:
						pack("PLAY2");
						zerujBx();
						break;
					case 5:
						pack("PLAY3");
						zerujBx();
						break;
					case 6:
						pack("W");
						zerujBx();
						break;



					}


				}
				else
				{
					pack("Er1");
					zerujBx();
				}


			}
			else {
								pack("Er2");
								zerujBx();
							}

		} else {
			pack("Er3");
			zerujBx();
		}
		}
		else{
			pack("Er4");
			zerujBx();
			zakres=1;

		}
	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 *
 * @retval None
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_SPI1_Init();
	/* USER CODE BEGIN 2 */

	HAL_UART_Receive_IT(&huart2, (uint8_t*) &Buff_Rx[0], 1);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	LCD_SCAN_DIR Lcd_ScanDir = SCAN_DIR_DFT; //SCAN_DIR_DFT = D2U_L2R
	LCD_Init(Lcd_ScanDir);
	start();

	while (1) {

		komunikat();
		volatile int menux = 3;
		volatile int menuy = 135;
		volatile int menulot = 1;
		while (gra == 1) {

			timer = 0;
			while (timer != 50) {
				ST7735_FillRectangle(menux, menuy, 3, 3, ST7735_YELLOW);

			}
			ST7735_FillRectangle(menux, menuy, 3, 3, ST7735_BLACK);

			menux++;
			switch (menulot) {
			case 1:
				menuy--;
				break;
			case 2:
				menuy++;
				break;
			}
			if (menuy == 115) {
				menulot = 2;
			} else if (menuy == 135) {
				menulot = 1;
			}
			if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0) {
				if (poziom == 0)
					lvl();
				if (poziom == 1)
					lvl2();
				if (poziom == 2)
					lvl3();

			}

			if (menux == 125) {
				menux = 3;
			}

			komunikat();

		}

		////////////////////////////////PIERWSZY POZIOM

		volatile int lvl1x = 3;
		volatile int lvl1y = 80;

		while (gra == 3) {

			timer = 0;
			while (timer != 100) {
				ST7735_FillRectangle(lvl1x, lvl1y, 3, 3, ST7735_YELLOW);

			}

			ST7735_FillRectangle(lvl1x, lvl1y, 3, 3, ST7735_BLACK);

			lvl1x++;
			lvl1y++;
			if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0) {
				lvl1x = lvl1x + 1;
				lvl1y = lvl1y - 5;

			}

			if ((lvl1y <= (-2)|| lvl1y >= 155)

					||((lvl1x==50||lvl1x==49||lvl1x==51||lvl1x==52||lvl1x==53)&&(lvl1y<=102 && lvl1y>=0))
					||((lvl1x==50||lvl1x==49||lvl1x==51||lvl1x==52||lvl1x==53)&&(lvl1y>=128 && lvl1y<=160))
					//druga
					||((lvl1x==65||lvl1x==64||lvl1x==66||lvl1x==67||lvl1x==68)&&(lvl1y<=112 && lvl1y>=0))
					||((lvl1x==65||lvl1x==64||lvl1x==67||lvl1x==67||lvl1x==68)&&(lvl1y>=138 && lvl1y<=160))
					//trzecia
					||((lvl1x==80||lvl1x==79||lvl1x==81||lvl1x==82||lvl1x==83)&&(lvl1y<=82 && lvl1y>=0))
					//czwarta

					||((lvl1x==100||lvl1x==99||lvl1x==101||lvl1x==102||lvl1x==103)&&(lvl1y>=78 && lvl1y<=160))



			)
			{
				wynikgry = 1;
				wynik();
				gra = 0;

			}
			if (lvl1x >= 125) {
				poziom = 1;
				start();
				gra = 1;
			}

			komunikat();

		}

		///////////////////////////////// DRUGI POZIOM
		volatile int lvl2x = 3;
		volatile int lvl2y = 80;

		while (gra == 4) {
			timer = 0;
			while (timer != 100) {
				ST7735_FillRectangle(lvl2x, lvl2y, 3, 3, ST7735_RED);

			}

			ST7735_FillRectangle(lvl2x, lvl2y, 3, 3, ST7735_BLACK);

			lvl2x++;
			lvl2y++;
			if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0) {
				lvl2x = lvl2x + 1;
				lvl2y = lvl2y - 5;

			}

			if ((lvl2y <= (-2) || lvl2y >= 155)
					||((lvl2x==30||lvl2x==29||lvl2x==31||lvl2x==32||lvl2x==33)&&(lvl2y<=97 && lvl2y>=0))
					||((lvl2x==30||lvl2x==29||lvl2x==31||lvl2x==32||lvl2x==33)&&(lvl2y>=113 && lvl2y<=160))
					///drugie
					||((lvl2x==50||lvl2x==49||lvl2x==51||lvl2x==52||lvl2x==53)&&(lvl2y<=82 && lvl2y>=0))
					||((lvl2x==50||lvl2x==49||lvl2x==51||lvl2x==52||lvl2x==53)&&(lvl2y>=98 && lvl2y<=160))
					//trzecie
					||((lvl2x==110||lvl2x==109||lvl2x==111||lvl2x==112||lvl2x==113)&&(lvl2y<=122 && lvl2y>=0))
					||((lvl2x==110||lvl2x==109||lvl2x==111||lvl2x==112||lvl2x==113)&&(lvl2y>=143 && lvl2y<=160))







			)

			{
				wynikgry = 2;
				wynik();
				gra = 0;

			}
			if (lvl2x >= 125) {
				poziom = 2;
				start();
				gra = 1;
			}

			komunikat();

		}

		//////////////////////POZIOM 3
		volatile int lvl3x = 3;
		volatile int lvl3y = 100;

		while (gra == 5) {
			timer = 0;
			while (timer != 100) {
				ST7735_FillRectangle(lvl3x, lvl3y, 3, 3, ST7735_MAGENTA);

			}

			ST7735_FillRectangle(lvl3x, lvl3y, 3, 3, ST7735_BLACK);

			lvl3x++;
			lvl3y++;
			if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0) {
				lvl3x = lvl3x + 1;
				lvl3y = lvl3y - 5;

			}

			if ((lvl3y <= (-2) || lvl3y >= 155)


				||((lvl3x==30||lvl3x==29||lvl3x==31||lvl3x==32||lvl3x==33)&&(lvl3y<=102 && lvl3y>=0))
				||((lvl3x==30||lvl3x==29||lvl3x==31||lvl3x==32||lvl3x==33)&&(lvl3y>=118 && lvl3y<=160))
				////druga

				||((lvl3x==45||lvl3x==44||lvl3x==46||lvl3x==47||lvl3x==48)&&(lvl3y<=92 && lvl3y>=0))
				||((lvl3x==45||lvl3x==44||lvl3x==46||lvl3x==47||lvl3x==48)&&(lvl3y>=108 && lvl3y<=160))
				//trzecia

				||((lvl3x==60||lvl3x==59||lvl3x==61||lvl3x==62||lvl3x==63)&&(lvl3y<=82 && lvl3y>=0))
				||((lvl3x==60||lvl3x==59||lvl3x==61||lvl3x==62||lvl3x==63)&&(lvl3y>=98 && lvl3y<=160))
				//czwarte

				||((lvl3x==75||lvl3x==75||lvl3x==75||lvl3x==75||lvl3x==75)&&(lvl3y<=72 && lvl3y>=0))
				||((lvl3x==75||lvl3x==75||lvl3x==75||lvl3x==75||lvl3x==75)&&(lvl3y>=88 && lvl3y<=160))
				//piate

				||((lvl3x==90||lvl3x==89||lvl3x==91||lvl3x==92||lvl3x==93)&&(lvl3y<=62 && lvl3y>=0))
				||((lvl3x==90||lvl3x==89||lvl3x==91||lvl3x==92||lvl3x==93)&&(lvl3y>=78 && lvl3y<=160))
				//szoste

				||((lvl3x==105||lvl3x==104||lvl3x==106||lvl3x==107||lvl3x==108)&&(lvl3y<=52 && lvl3y>=0))
				||((lvl3x==105||lvl3x==104||lvl3x==106||lvl3x==107||lvl3x==108)&&(lvl3y>=68 && lvl3y<=160))

				)
			{
				wynikgry = 3;
				wynik();
				gra = 0;

			}
			if (lvl3x >= 125) {
				poziom = 3;
				start();
				gra = 1;
			}

			komunikat();

		}

		while (gra == 2) {
			stop();
			komunikat();

		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

	}
	/* USER CODE END 3 */

}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	/**Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE()
	;

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure the Systick interrupt time
	 */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

	/**Configure the Systick
	 */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* SPI1 init function */
static void MX_SPI1_Init(void) {

	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* USART2 init function */
static void MX_USART2_UART_Init(void) {

	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

}

/** Configure pins as 
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
 */
static void MX_GPIO_Init(void) {

	GPIO_InitTypeDef GPIO_InitStruct;

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE()
	;
	__HAL_RCC_GPIOH_CLK_ENABLE()
	;
	__HAL_RCC_GPIOA_CLK_ENABLE()
	;
	__HAL_RCC_GPIOB_CLK_ENABLE()
	;

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : RS_Pin */
	GPIO_InitStruct.Pin = RS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(RS_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : RST_Pin */
	GPIO_InitStruct.Pin = RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(RST_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : CS_Pin */
	GPIO_InitStruct.Pin = CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CS_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  file: The file name as string.
 * @param  line: The line in file as a number.
 * @retval None
 */
void _Error_Handler(char *file, int line) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
