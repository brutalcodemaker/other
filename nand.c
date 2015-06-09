/**
 *	Драйвер встроенного контроллера внешней памяти (в режиме NAND).
 *	Микросхема MT29F16G08x производства фирмы Micron.
 */

#include <opora.h>

/**
 *	Функция инициализирует контроллер внешней NAND-Flash памяти
 *
 *	Не возвращает параметров
 */
void nand_init(void)
{
	// Разрешить тактирование EXT_BUS, PORTA, PORTC
	RST_CLK->PER_CLOCK |= 0x40A00000;

	// Настроить линии контроллера внешней шины
	PORTA->FUNC   |= 0x5555;
	PORTA->ANALOG |= 0x80FF;
	PORTA->PWR    |= 0xC000FFFF;
	// Настроить вывод PA15 как выход (сигнал ChipSelect)
	PORTA->OE     |= 0x8000;

	PORTC->FUNC   |= 0x0295;
	PORTC->ANALOG |= 0x1F;
	PORTC->PWR    |= 0x03FF;

	// Выбрать тип микросхемы памяти (NAND)
	EXT_BUS->EXT_BUS_CONTROL = 0x04;
	// Задать временные параметры чтения/записи для частоты 120 МГц
	EXT_BUS->NAND_CYCLES     = 0x03C83588;

	// Выполнить сброс микросхемы памяти (0xFF)
	*(volatile unsigned char*)0x770007F8 = 0;
	// Ждать окончания выполнения команды
	while ( (EXT_BUS->EXT_BUS_CONTROL & 0x80) == 0 );
}

/**
 *	Функция считывает страницу параметров микросхемы памяти
 *
 *	page - буфер для считывания страницы параметров (768 байтов)
 *
 *	Не возвращает параметров
 */
void nand_get_param_page(unsigned char *page)
{
	int i;

	// Команда чтения страницы параметров микросхемы (0xEC)
	*(volatile unsigned char*)0x77200760 = 0;
	// Ждать окончания выполнения команды
	while ( (EXT_BUS->EXT_BUS_CONTROL & 0x80) == 0 );

	// Считать данные страницы параметров
	for (i = 0; i < 768; i++)
		page[ i ] = *(volatile unsigned char*)0x77280000;
}

/**
 *	Функция считывает флаги состояния микросхемы памяти
 *
 *	Возвращает байт состояния микросхемы памяти
 */
int nand_get_status(void)
{
	// Команда чтения регистра статуса микросхемы (0x90)
	*(volatile unsigned char*)0x77000380 = 0;
	// Вернуть значение регистра
	return *(volatile unsigned char*)0x77280000;
}

/**
 *	Функция выполняет стирание блока памяти (2048 страниц)
 *
 *	addr - адрес внутри стираемого блока
 *
 *	Возвращает состояние микросхемы после выполнения операции стирания
 */
int nand_erase(int addr)
{
	// Команда стирания блока памяти в микросхеме (0x60-0xD0)
	*(volatile unsigned char*)0x77768300 = ( addr >> 13 ) & 0xFF;
	*(volatile unsigned char*)0x77768300 = ( addr >> 21 ) & 0xFF;
	*(volatile unsigned char*)0x77768300 = ( addr >> 29 ) & 0x07;

	// Ждать окончания выполнения команды
	while ( (EXT_BUS->EXT_BUS_CONTROL & 0x80) == 0 );

	return nand_get_status();
}

/**
 *	Функция считывает массив данных из микросхемы памяти
 *
 *	addr - начальный адрес чтения данных
 *	len  - кол-во считываемых байтов
 *	buf  - буфер для считывания данных из микросхемы памяти
 *
 *	Не возвращает параметров
 */
void nand_read(int addr, int len, unsigned char *buf)
{
	int i;

	// Команда чтения страницы памяти из микросхемы (0x30-0x00)
	*(volatile unsigned char*)0x77B18000 = addr & 0xFF;
	*(volatile unsigned char*)0x77B18000 = ( addr >> 8 )  & 0x1F;
	*(volatile unsigned char*)0x77B18000 = ( addr >> 13 ) & 0xFF;
	*(volatile unsigned char*)0x77B18000 = ( addr >> 21 ) & 0xFF;
	*(volatile unsigned char*)0x77B18000 = ( addr >> 29 ) & 0x07;

	// Ждать окончания выполнения команды
	while ( (EXT_BUS->EXT_BUS_CONTROL & 0x80) == 0 );

	// Считать данные страницы
	for (i = 0; i < len; i++)
		buf[ i ] = *(volatile unsigned char*)0x77280000;
}

/**
 *	Функция записывает массив данных в микросхему памяти
 *
 *	addr - начальный адрес записи данных
 *	len  - кол-во записываемых байтов
 *	buf  - буфер записываемых данных в микросхему памяти
 *
 *	Не возвращает параметров
 */
void nand_write(int addr, int len, unsigned char *buf)
{
	int i;

	// Команда записи страницы памяти в микросхему (0x80)
	*(volatile unsigned char*)0x77A00400 = addr & 0xFF;
	*(volatile unsigned char*)0x77A00400 = ( addr >> 8 )  & 0x1F;
	*(volatile unsigned char*)0x77A00400 = ( addr >> 13 ) & 0xFF;
	*(volatile unsigned char*)0x77A00400 = ( addr >> 21 ) & 0xFF;
	*(volatile unsigned char*)0x77A00400 = ( addr >> 29 ) & 0x07;

	len--;
	// Записать все байты кроме последнего
	for (i = 0; i < len; i++)
		*(volatile unsigned char*)0x77288000 = buf[ i ];

	// Завершить операцию записи (0x10)
	*(volatile unsigned char*)0x77388000 = buf[ i ];
	// Ждать окончания выполнения команды
	while ( (EXT_BUS->EXT_BUS_CONTROL & 0x80) == 0 );
}
